extern "C" {
#include "py/builtin.h"
#include <py/runtime.h>
#include <py/obj.h>
#include <py/stream.h>
}
#include "modkandinsky.h"
#include <py/misc.h>
#include <escher/palette.h>
#include <kandinsky.h>
#include <kandinsky/framebuffer.h>
#include <kandinsky/framebuffer_context.h>
#include <kandinsky/ion_context.h>
#include <ion.h>
#include "port.h"

using namespace MicroPython;

#include <new>
#include <stdint.h>
#include <limits.h>
#include <cmath>
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif
#include <stdlib.h>
#include <string.h>

// Conversion table RGB222 (6 bits packed as RRGGBB) -> KDColor (RGB565)
static KDColor rgb222_to_kdcolor[64];
static KDColor rgb332_to_kdcolor[256];
static bool rgb222_table_initialized = false;
static bool rgb332_table_initialized = false;

static void init_rgb222_table() {
  if (rgb222_table_initialized) return;
  for (int i = 0; i < 64; i++) {
    uint8_t r2 = (i >> 4) & 0x3;
    uint8_t g2 = (i >> 2) & 0x3;
    uint8_t b2 = i & 0x3;
    uint8_t r5 = (uint8_t)((r2 * 31 + 1) / 3);
    uint8_t g6 = (uint8_t)((g2 * 63 + 1) / 3);
    uint8_t b5 = (uint8_t)((b2 * 31 + 1) / 3);
    uint16_t rgb16 = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
    rgb222_to_kdcolor[i] = KDColor::RGB16(rgb16);
  }
  rgb222_table_initialized = true;
}

static void init_rgb332_table() {
  if (rgb332_table_initialized) return;
  for (int i = 0; i < 256; i++) {
    uint8_t r3 = (i >> 5) & 0x7;
    uint8_t g3 = (i >> 2) & 0x7;
    uint8_t b2 = i & 0x3;
    uint8_t r5 = (uint8_t)((r3 * 31 + 3) / 7);
    uint8_t g6 = (uint8_t)((g3 * 63 + 3) / 7);
    uint8_t b5 = (uint8_t)((b2 * 31 + 1) / 3);
    uint16_t rgb16 = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
    rgb332_to_kdcolor[i] = KDColor::RGB16(rgb16);
  }
  rgb332_table_initialized = true;
}

static inline uint8_t kdcolor_to_rgb222(KDColor c) {
  uint8_t r2 = c.red() >> 6; // 0..3
  uint8_t g2 = c.green() >> 6; // 0..3
  uint8_t b2 = c.blue() >> 6; // 0..3
  return (uint8_t)((r2 << 4) | (g2 << 2) | b2);
}

// Packed pixel helpers for 6 bits per pixel
// Generic packed pixel helpers for arbitrary bits per pixel (1..16)
static inline uint32_t packed_get_pixel_bits(const uint8_t *buf, size_t idx, uint8_t bpp) {
  size_t bit = idx * (size_t)bpp;
  size_t b = bit >> 3;
  int off = bit & 7;
  uint32_t v = ((uint32_t)buf[b] >> off);
  // read next bytes if necessary
  if (off + bpp > 8) {
    v |= (uint32_t)buf[b+1] << (8 - off);
    if (off + bpp > 16) {
      v |= (uint32_t)buf[b+2] << (16 - off);
    }
  }
  uint32_t mask = ((1u << bpp) - 1u);
  return v & mask;
}

static inline void packed_set_pixel_bits(uint8_t *buf, size_t idx, uint32_t value, uint8_t bpp) {
  size_t bit = idx * (size_t)bpp;
  size_t b = bit >> 3;
  int off = bit & 7;
  uint32_t d = (uint32_t)buf[b] | ((uint32_t)buf[b+1] << 8) | ((uint32_t)buf[b+2] << 16);
  uint32_t mask = ((1u << bpp) - 1u) << off;
  d = (d & ~mask) | (((value & ((1u << bpp) - 1u)) << off) & mask);
  buf[b] = d & 0xFF;
  buf[b+1] = (d >> 8) & 0xFF;
  buf[b+2] = (d >> 16) & 0xFF;
}

static mp_obj_t TupleForKDColor(KDColor c) {
  mp_obj_tuple_t * t = static_cast<mp_obj_tuple_t *>(MP_OBJ_TO_PTR(mp_obj_new_tuple(3, NULL)));
  t->items[0] = MP_OBJ_NEW_SMALL_INT(c.red());
  t->items[1] = MP_OBJ_NEW_SMALL_INT(c.green());
  t->items[2] = MP_OBJ_NEW_SMALL_INT(c.blue());
  return MP_OBJ_FROM_PTR(t);
}

// Helper: read exactly `len` bytes from stream, else raise RuntimeError
static void stream_read_all(mp_obj_t stream, const mp_stream_p_t *stream_p, void *buf, size_t len, int *error) {
  size_t read = 0;
  while (read < len) {
    mp_int_t r = stream_p->read(stream, (char *)buf + read, len - read, error);
    if (r <= 0) {
      stream_p->ioctl(stream, MP_STREAM_CLOSE, 0, error);
      mp_raise_msg(&mp_type_RuntimeError, "corrupted file");
    }
    read += (size_t)r;
  }
}

// Helper: write exactly `len` bytes to stream, else raise RuntimeError
static void stream_write_all(mp_obj_t stream, const mp_stream_p_t *stream_p, const void *buf, size_t len, int *error) {
  size_t written = 0;
  while (written < len) {
    mp_int_t w = stream_p->write(stream, (const char *)buf + written, len - written, error);
    if (w <= 0) {
      stream_p->ioctl(stream, MP_STREAM_CLOSE, 0, error);
      mp_raise_msg(&mp_type_RuntimeError, "stream write error");
    }
    written += (size_t)w;
  }
}


// Parse either a KDColor or a palette index (int) when framebuffer is in palette mode

typedef struct _kandinsky_framebuffer_obj_t {
  mp_obj_base_t base;
  /* pixels are stored as packed RGB222 (6 bits per pixel) */
  uint8_t * pixels;
  KDSize size;
  bool freed;
  uint8_t bitsPerPixel;
  uint8_t format; /* 0=palette, 1=rgb222, 2=rgb332, 3=rgb565 */
  KDColor * palette;
  size_t palette_len;
  size_t palette_capacity;
} kandinsky_framebuffer_obj_t;

// Now that the framebuffer struct is defined, implement the color-or-palette parser
static inline KDColor parse_color_or_palette(kandinsky_framebuffer_obj_t *self, mp_obj_t obj) {
  if (mp_obj_is_int(obj)) {
    if (self->format != 0) {
      mp_raise_TypeError("Int are not colors");
    }
    mp_int_t idx = mp_obj_get_int(obj);
    if (idx < 0 || (size_t)idx >= self->palette_len) {
      mp_raise_msg(&mp_type_IndexError, "palette index out of range");
    }
    return self->palette[(size_t)idx];
  }
  return MicroPython::Color::Parse(obj);
}

// Find the closest color index in the palette to a given KDColor
static inline size_t palette_find_closest_index(kandinsky_framebuffer_obj_t *self, KDColor c) {
  if (!self->palette || self->palette_len == 0) return 0;
  size_t best = 0;
  uint32_t bestDist = (uint32_t)-1;
  for (size_t i = 0; i < self->palette_len; i++) {
    int dr = (int)c.red() - (int)self->palette[i].red();
    int dg = (int)c.green() - (int)self->palette[i].green();
    int db = (int)c.blue() - (int)self->palette[i].blue();
    uint32_t d = (uint32_t)(dr*dr + dg*dg + db*db);
    if (d < bestDist) { bestDist = d; best = i; if (d == 0) break; }
  }
  return best;
}

static inline KDColor framebuffer_color_from_index(kandinsky_framebuffer_obj_t *self, uint32_t v) {
  if (self->format == 1) {
    return rgb222_to_kdcolor[v];
  }
  if (self->format == 2) {
    return rgb332_to_kdcolor[v];
  }
  if (self->format == 3) {
    return KDColor::RGB16((uint16_t)v);
  }
  return (v < self->palette_len) ? self->palette[v] : KDColor::RGB16(0);
}

static inline uint32_t framebuffer_index_from_color(kandinsky_framebuffer_obj_t *self, KDColor c, bool requireExactPaletteColor) {
  if (self->format == 1) {
    return (uint32_t)kdcolor_to_rgb222(c);
  }
  if (self->format == 2) {
    return (uint32_t)(((c.red() >> 5) << 5) | ((c.green() >> 5) << 2) | (c.blue() >> 6));
  }
  if (self->format == 3) {
    return (uint32_t)(((c.red() >> 3) << 11) | ((c.green() >> 2) << 5) | (c.blue() >> 3));
  }
  if (requireExactPaletteColor) {
    for (size_t i = 0; i < self->palette_len; i++) {
      if (self->palette[i].red() == c.red() && self->palette[i].green() == c.green() && self->palette[i].blue() == c.blue()) {
        return (uint32_t)i;
      }
    }
    mp_raise_ValueError("color not in palette");
  }
  return (uint32_t)palette_find_closest_index(self, c);
}

static void framebuffer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
  (void)kind;
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(self_in);
  mp_print_str(print, "<kandinsky.framebuffer ");
  mp_print_str(print, "@");
  mp_printf(print, "(%dx%d)", self->size.width(), self->size.height());
  mp_print_str(print, ">");
}
// Constructor: framebuffer(w, h, bits=6, palette=False)
STATIC mp_obj_t framebuffer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
  // framebuffer(w, h) -> defaults to rgb222 (6 bits)
  // framebuffer(w, h, bits) -> choose format by bits: 6=rgb222,8=rgb332,16=rgb565
  // framebuffer(w, h, bits, palette=True) -> force palette mode with 2**bits entries
  mp_arg_check_num(n_args, n_kw, 2, 4, false);
  int w = mp_obj_get_int(args[0]);
  int h = mp_obj_get_int(args[1]);
  int bits = 6;
  bool palette = false;
  if (n_args >= 3) {
    bits = mp_obj_get_int(args[2]);
  }
  if (n_args >= 4) {
    palette = mp_obj_is_true(args[3]);
  }
  if (w <= 0 || h <= 0) {
    mp_raise_ValueError("width and height must be > 0");
  }

  /* check multiplication overflow for width*height */
  size_t sw = (size_t)w;
  size_t sh = (size_t)h;
  if (sw > 0 && sh > (size_t)SIZE_MAX / sw) {
    mp_raise_ValueError("width*height overflow");
  }
  size_t area = sw * sh;

  /* allocate packed pixel buffer (RGB222: 6 bits per pixel) */
  // allocate packed pixel buffer according to bits per pixel
  if (bits <= 0 || bits > 16) mp_raise_ValueError("bits_per_pixel must be between 1 and 16");
  size_t data_size_bits = (area * bits + 7) / 8;
  uint8_t * pixels = static_cast<uint8_t *>(m_malloc(data_size_bits + 2));
  if (!pixels) {
    mp_raise_msg(&mp_type_MemoryError, "not enough memory for pixels");
  }
  /* initialize to zero (black) and add an extra zero byte to simplify reads */
  memset(pixels, 0, data_size_bits + 2);

  /* now create Python object and transfer ownership */
  kandinsky_framebuffer_obj_t * o = m_new_obj_with_finaliser(kandinsky_framebuffer_obj_t);
  o->base.type = (mp_obj_type_t*)&kandinsky_framebuffer_type;
  o->size = KDSize(w, h);
  o->pixels = pixels;
  o->freed = false;
  o->palette = nullptr;
  o->palette_len = 0;
  o->palette_capacity = 0;
  o->bitsPerPixel = (uint8_t)bits;
  if (!palette && bits == 6) {
    o->format = 1; /* rgb222 */
  } else if (!palette && bits == 8) {
    o->format = 2; /* rgb332 */
  } else if (!palette && bits == 16) {
    o->format = 3; /* rgb565 */
  } else {
    o->format = 0; /* palette mode */
    size_t maxColors = (size_t)1 << bits;
    // allocate palette storage
    o->palette_capacity = maxColors;
    o->palette = static_cast<KDColor *>(m_malloc(maxColors * sizeof(KDColor)));
    if (!o->palette) {
      m_free(o->pixels);
      mp_raise_msg(&mp_type_MemoryError, "not enough memory for palette");
    }
    // initialize palette with black entries to avoid uninitialized colors
    for (size_t pi = 0; pi < maxColors; pi++) {
      o->palette[pi] = KDColor::RGB16(0);
    }
    o->palette_len = maxColors;
  }

  return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t framebuffer_fill(mp_obj_t self_in, mp_obj_t color_obj) {
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(self_in);
  init_rgb222_table();
  init_rgb332_table();
  int w = self->size.width();
  int h = self->size.height();
  size_t area = (size_t)w * (size_t)h;
  uint32_t colIndex;
  if (mp_obj_is_int(color_obj)) {
    colIndex = (uint32_t)mp_obj_get_int(color_obj);
  } else {
    KDColor c = MicroPython::Color::Parse(color_obj);
    // convert to index depending on format
    if (self->format == 1) {
      colIndex = (uint32_t)kdcolor_to_rgb222(c);
    } else if (self->format == 2) {
      uint8_t r3 = c.red() >> 5;
      uint8_t g3 = c.green() >> 5;
      uint8_t b2 = c.blue() >> 6;
      colIndex = (uint32_t)((r3<<5)|(g3<<2)|b2);
    } else if (self->format == 3) {
      uint16_t r5 = c.red() >> 3;
      uint16_t g6 = c.green() >> 2;
      uint16_t b5 = c.blue() >> 3;
      colIndex = (uint32_t)((r5<<11)|(g6<<5)|b5);
    } else {
      // palette: find exact match
      bool found = false;
      for (size_t i = 0; i < self->palette_len; i++) {
        if (self->palette[i].red() == c.red() && self->palette[i].green() == c.green() && self->palette[i].blue() == c.blue()) {
          colIndex = (uint32_t)i;
          found = true;
          break;
        }
      }
      if (!found) {
        mp_raise_ValueError("color not in palette");
      }
    }
  }
  uint8_t bpp = self->bitsPerPixel;
  for (size_t i = 0; i < area; i++) {
    packed_set_pixel_bits(self->pixels, i, colIndex, bpp);
  }
  return mp_const_none;
}
const mp_obj_fun_builtin_fixed_t framebuffer_fill_obj = {
  {&mp_type_fun_builtin_2},
  {(mp_fun_0_t)framebuffer_fill}
};

// change_color(self, index, (r,g,b)) -> sets palette[index]
STATIC mp_obj_t framebuffer_change_color(size_t n_args, const mp_obj_t *args) {
  // args: self, index, color
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(args[0]);
  if (self->format != 0) {
    mp_raise_ValueError("change_color only valid for palette framebuffers");
  }
  mp_int_t idx = mp_obj_get_int(args[1]);
  if (idx < 0 || (size_t)idx >= self->palette_capacity) {
    mp_raise_msg(&mp_type_IndexError, "palette index out of range");
  }
  KDColor c = parse_color_or_palette(self, args[2]);
  self->palette[(size_t)idx] = c;
  return mp_const_none;
}
const mp_obj_fun_builtin_var_t framebuffer_change_color_obj = {
  {&mp_type_fun_builtin_var},
  MP_OBJ_FUN_MAKE_SIG(3,3,false),
  {(mp_fun_var_t)framebuffer_change_color}
};

// Note: `update()` removed — use `draw()` which handles blitting.

STATIC mp_obj_t framebuffer_set_pixel(size_t n_args, const mp_obj_t *args) {
  // args: self, x, y, color
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(args[0]);
  int x = mp_obj_get_int(args[1]);
  int y = mp_obj_get_int(args[2]);
  if (x < 0 || y < 0 || x >= self->size.width() || y >= self->size.height()) {
    mp_raise_msg(&mp_type_IndexError, "pixel out of range");
  }
  init_rgb222_table();
  uint32_t colIndex;
  if (mp_obj_is_int(args[3])) {
    colIndex = (uint32_t)mp_obj_get_int(args[3]);
  } else {
    KDColor c = parse_color_or_palette(self, args[3]);
    if (self->format == 1) colIndex = (uint32_t)kdcolor_to_rgb222(c);
    else if (self->format == 2) colIndex = (uint32_t)(((c.red()>>5)<<5)|((c.green()>>5)<<2)|(c.blue()>>6));
    else if (self->format == 3) colIndex = (uint32_t)(((c.red()>>3)<<11)|((c.green()>>2)<<5)|(c.blue()>>3));
    else {
      bool found = false;
      for (size_t i = 0; i < self->palette_len; i++) {
        if (self->palette[i].red() == c.red() && self->palette[i].green() == c.green() && self->palette[i].blue() == c.blue()) { colIndex = (uint32_t)i; found = true; break; }
      }
      if (!found) mp_raise_ValueError("color not in palette");
    }
  }
  packed_set_pixel_bits(self->pixels, (size_t)y * self->size.width() + x, colIndex, self->bitsPerPixel);
  return mp_const_none;
}
const mp_obj_fun_builtin_var_t framebuffer_set_pixel_obj = {
  {&mp_type_fun_builtin_var},
  MP_OBJ_FUN_MAKE_SIG(4, 4, false),
  {(mp_fun_var_t)framebuffer_set_pixel}
};

STATIC mp_obj_t framebuffer_get_pixel(mp_obj_t self_in, mp_obj_t x_obj, mp_obj_t y_obj) {
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(self_in);
  int x = mp_obj_get_int(x_obj);
  int y = mp_obj_get_int(y_obj);
  if (x < 0 || y < 0 || x >= self->size.width() || y >= self->size.height()) {
    mp_raise_msg(&mp_type_IndexError, "pixel out of range");
  }
  init_rgb222_table();
  uint32_t v = packed_get_pixel_bits(self->pixels, (size_t)y * self->size.width() + x, self->bitsPerPixel);
  if (self->format == 1) {
    return TupleForKDColor(rgb222_to_kdcolor[v]);
  } else if (self->format == 2) {
    return TupleForKDColor(rgb332_to_kdcolor[v]);
  } else if (self->format == 3) {
    return TupleForKDColor(KDColor::RGB16((uint16_t)v));
  } else {
    if (v >= self->palette_len) mp_raise_msg(&mp_type_IndexError, "palette index out of range");
    return TupleForKDColor(self->palette[v]);
  }
}
const mp_obj_fun_builtin_fixed_t framebuffer_get_pixel_obj = {
  {&mp_type_fun_builtin_3},
  {(mp_fun_0_t)framebuffer_get_pixel}
};

// debug: return palette entry at index
STATIC mp_obj_t framebuffer_get_palette_entry(mp_obj_t self_in, mp_obj_t idx_obj) {
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(self_in);
  if (self->format != 0) mp_raise_ValueError("not a palette framebuffer");
  mp_int_t idx = mp_obj_get_int(idx_obj);
  if (idx < 0 || (size_t)idx >= self->palette_len) mp_raise_msg(&mp_type_IndexError, "palette index out of range");
  return TupleForKDColor(self->palette[(size_t)idx]);
}
const mp_obj_fun_builtin_fixed_t framebuffer_get_palette_entry_obj = {
  {&mp_type_fun_builtin_2},
  {(mp_fun_0_t)framebuffer_get_palette_entry}
};

// debug: return raw palette/index value at x,y
STATIC mp_obj_t framebuffer_get_raw_index(mp_obj_t self_in, mp_obj_t x_obj, mp_obj_t y_obj) {
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(self_in);
  int x = mp_obj_get_int(x_obj);
  int y = mp_obj_get_int(y_obj);
  if (x < 0 || y < 0 || x >= self->size.width() || y >= self->size.height()) {
    mp_raise_msg(&mp_type_IndexError, "pixel out of range");
  }
  uint32_t v = packed_get_pixel_bits(self->pixels, (size_t)y * self->size.width() + x, self->bitsPerPixel);
  return mp_obj_new_int((mp_int_t)v);
}
const mp_obj_fun_builtin_fixed_t framebuffer_get_raw_index_obj = {
  {&mp_type_fun_builtin_3},
  {(mp_fun_0_t)framebuffer_get_raw_index}
};

// __del__/close: free allocated C++ resources
STATIC mp_obj_t framebuffer_close(mp_obj_t self_in) {
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(self_in);
  if (!self->freed) {
    self->freed = true;
    if (self->pixels) {
      m_free(self->pixels);
      self->pixels = nullptr;
    }
    if (self->palette) {
      m_free(self->palette);
      self->palette = nullptr;
      self->palette_len = 0;
      self->palette_capacity = 0;
    }
  }
  return mp_const_none;
}
const mp_obj_fun_builtin_fixed_t framebuffer_close_obj = {
  {&mp_type_fun_builtin_1},
  {(mp_fun_0_t)framebuffer_close}
};

// get_pixels(self, x, y, w, h) -> [[(r,g,b),...], ...]
STATIC mp_obj_t framebuffer_get_pixels(size_t n_args, const mp_obj_t *args) {
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(args[0]);
  int x = mp_obj_get_int(args[1]);
  int y = mp_obj_get_int(args[2]);
  int w = mp_obj_get_int(args[3]);
  int h = mp_obj_get_int(args[4]);
  if (w <= 0 || h <= 0) mp_raise_ValueError("width and height must be > 0");
  int fbw = self->size.width();
  int fbh = self->size.height();
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x >= fbw || y >= fbh) return mp_obj_new_list(0, NULL);
  if (x + w > fbw) w = fbw - x;
  if (y + h > fbh) h = fbh - y;
  if (w <= 0 || h <= 0) return mp_obj_new_list(0, NULL);
  init_rgb222_table();
  init_rgb332_table();
  mp_obj_t outer = mp_obj_new_list(0, NULL);
  for (int j = 0; j < h; j++) {
    mp_obj_t row = mp_obj_new_list(0, NULL);
    for (int i = 0; i < w; i++) {
      size_t idx = (size_t)(y + j) * fbw + (x + i);
      uint32_t v = packed_get_pixel_bits(self->pixels, idx, self->bitsPerPixel);
      KDColor c;
      if (self->format == 1) c = rgb222_to_kdcolor[v];
      else if (self->format == 2) c = rgb332_to_kdcolor[v];
      else if (self->format == 3) c = KDColor::RGB16((uint16_t)v);
      else c = (v < self->palette_len) ? self->palette[v] : KDColor::RGB16(0);
      mp_obj_list_append(row, TupleForKDColor(c));
    }
    mp_obj_list_append(outer, row);
  }
  return outer;
}
const mp_obj_fun_builtin_var_t framebuffer_get_pixels_obj = {
  {&mp_type_fun_builtin_var},
  MP_OBJ_FUN_MAKE_SIG(5,5,false),
  {(mp_fun_var_t)framebuffer_get_pixels}
};

// save/load to file removed: functionality not reliable on device

// draw_line(self, x1, y1, x2, y2, color)
STATIC mp_obj_t framebuffer_draw_line(size_t n_args, const mp_obj_t *args) {
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(args[0]);
  mp_int_t x1 = mp_obj_get_int(args[1]);
  mp_int_t y1 = mp_obj_get_int(args[2]);
  mp_int_t x2 = mp_obj_get_int(args[3]);
  mp_int_t y2 = mp_obj_get_int(args[4]);
  KDColor color = parse_color_or_palette(self, args[5]);
  init_rgb222_table();
  int w = self->size.width();
  int h = self->size.height();
  int minx = x1 < x2 ? x1 : x2;
  int miny = y1 < y2 ? y1 : y2;
  int maxx = x1 > x2 ? x1 : x2;
  int maxy = y1 > y2 ? y1 : y2;
  if (minx < 0) minx = 0;
  if (miny < 0) miny = 0;
  if (maxx >= w) maxx = w-1;
  if (maxy >= h) maxy = h-1;
  if (minx > maxx || miny > maxy) return mp_const_none;
  int rectw = maxx - minx + 1;
  int recth = maxy - miny + 1;
  KDColor * tmp = static_cast<KDColor *>(m_malloc(rectw * recth * sizeof(KDColor)));
  if (!tmp) mp_raise_msg(&mp_type_MemoryError, "not enough memory for draw buffer");
  for (int j = 0; j < recth; j++) {
    for (int i = 0; i < rectw; i++) {
      size_t idx = (size_t)(miny + j) * w + (minx + i);
      uint32_t pix = packed_get_pixel_bits(self->pixels, idx, self->bitsPerPixel);
      if (self->format == 1) tmp[j*rectw + i] = rgb222_to_kdcolor[pix];
      else if (self->format == 2) tmp[j*rectw + i] = rgb332_to_kdcolor[pix];
      else if (self->format == 3) tmp[j*rectw + i] = KDColor::RGB16((uint16_t)pix);
      else tmp[j*rectw + i] = (pix < self->palette_len) ? self->palette[pix] : KDColor::RGB16(0);
    }
  }
  KDFrameBuffer fb(tmp, KDSize(rectw, recth));
  KDFrameBufferContext ctx(&fb);
  KDPoint p1_local(x1 - minx, y1 - miny);
  KDPoint p2_local(x2 - minx, y2 - miny);
  ctx.drawLine(p1_local, p2_local, color);
  /* write back compressed */
  for (int j = 0; j < recth; j++) {
    for (int i = 0; i < rectw; i++) {
      size_t idx = (size_t)(miny + j) * w + (minx + i);
      uint32_t idxColor;
      if (self->format == 1) idxColor = (uint32_t)kdcolor_to_rgb222(tmp[j*rectw + i]);
      else if (self->format == 2) {
        uint8_t r3 = tmp[j*rectw + i].red() >> 5;
        uint8_t g3 = tmp[j*rectw + i].green() >> 5;
        uint8_t b2 = tmp[j*rectw + i].blue() >> 6;
        idxColor = (uint32_t)((r3<<5)|(g3<<2)|b2);
      } else if (self->format == 3) {
        uint16_t r5 = tmp[j*rectw + i].red() >> 3;
        uint16_t g6 = tmp[j*rectw + i].green() >> 2;
        uint16_t b5 = tmp[j*rectw + i].blue() >> 3;
        idxColor = (uint32_t)((r5<<11)|(g6<<5)|b5);
      } else {
        idxColor = (uint32_t)palette_find_closest_index(self, tmp[j*rectw + i]);
      }
      packed_set_pixel_bits(self->pixels, idx, idxColor, self->bitsPerPixel);
    }
  }
  m_free(tmp);
  return mp_const_none;
}
const mp_obj_fun_builtin_var_t framebuffer_draw_line_obj = {
  {&mp_type_fun_builtin_var},
  MP_OBJ_FUN_MAKE_SIG(6, 6, false),
  {(mp_fun_var_t)framebuffer_draw_line}
};

// draw_string(self, text, x, y, textColor?, backgroundColor?, smallFont?, italic?)
STATIC mp_obj_t framebuffer_draw_string(size_t n_args, const mp_obj_t * args) {
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(args[0]);
  const char * text = mp_obj_str_get_str(args[1]);
  KDPoint point(mp_obj_get_int(args[2]), mp_obj_get_int(args[3]));
  KDColor textColor = (n_args >= 5) ? parse_color_or_palette(self, args[4]) : Palette::PrimaryText;
  KDColor backgroundColor = (n_args >= 6) ? parse_color_or_palette(self, args[5]) : Palette::HomeBackground;
  bool smallFont = (n_args >= 7) ? mp_obj_is_true(args[6]) : false;
  bool isItalic = (n_args >= 8) ? mp_obj_is_true(args[7]) : false;
  const KDFont * font = (!smallFont && !isItalic) ? KDFont::LargeFont : (!smallFont && isItalic ? KDFont::ItalicLargeFont : (smallFont && !isItalic ? KDFont::SmallFont : KDFont::ItalicSmallFont));
  init_rgb222_table();
  int x = point.x();
  int y = point.y();
  int w = self->size.width();
  int h = self->size.height();
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  // Measure text to avoid allocating a full-screen buffer when not needed
  KDSize textSize = font->stringSize(text);
  int rectw = textSize.width();
  int recth = textSize.height();
  if (rectw <= 0 || recth <= 0) return mp_const_none;
  if (x + rectw > w) rectw = w - x;
  if (y + recth > h) recth = h - y;
  if (rectw <= 0 || recth <= 0) return mp_const_none;
  init_rgb332_table();
  KDColor * tmp = static_cast<KDColor *>(m_malloc(rectw * recth * sizeof(KDColor)));
  if (!tmp) mp_raise_msg(&mp_type_MemoryError, "not enough memory for draw buffer");
  for (int j = 0; j < recth; j++) {
    for (int i = 0; i < rectw; i++) {
      size_t idx = (size_t)(y + j) * w + (x + i);
      uint32_t pix = packed_get_pixel_bits(self->pixels, idx, self->bitsPerPixel);
      if (self->format == 1) tmp[j*rectw + i] = rgb222_to_kdcolor[pix];
      else if (self->format == 2) tmp[j*rectw + i] = rgb332_to_kdcolor[pix];
      else if (self->format == 3) tmp[j*rectw + i] = KDColor::RGB16((uint16_t)pix);
      else tmp[j*rectw + i] = (pix < self->palette_len) ? self->palette[pix] : KDColor::RGB16(0);
    }
  }
  KDFrameBuffer fb(tmp, KDSize(rectw, recth));
  KDFrameBufferContext ctx(&fb);
  ctx.drawString(text, KDPoint(0,0), font, textColor, backgroundColor);
  /* write back compressed */
  for (int j = 0; j < recth; j++) {
    for (int i = 0; i < rectw; i++) {
      size_t idx = (size_t)(y + j) * w + (x + i);
      uint32_t idxColor;
      if (self->format == 1) idxColor = (uint32_t)kdcolor_to_rgb222(tmp[j*rectw + i]);
      else if (self->format == 2) idxColor = (uint32_t)(((tmp[j*rectw + i].red()>>5)<<5)|((tmp[j*rectw + i].green()>>5)<<2)|(tmp[j*rectw + i].blue()>>6));
      else if (self->format == 3) idxColor = (uint32_t)(((tmp[j*rectw + i].red()>>3)<<11)|((tmp[j*rectw + i].green()>>2)<<5)|(tmp[j*rectw + i].blue()>>3));
      else {
        idxColor = (uint32_t)palette_find_closest_index(self, tmp[j*rectw + i]);
      }
      packed_set_pixel_bits(self->pixels, idx, idxColor, self->bitsPerPixel);
    }
  }
  m_free(tmp);
  return mp_const_none;
}
const mp_obj_fun_builtin_var_t framebuffer_draw_string_obj = {
  {&mp_type_fun_builtin_var},
  MP_OBJ_FUN_MAKE_SIG(4, 8, false),
  {(mp_fun_var_t)framebuffer_draw_string}
};

// draw_circle(self, x, y, r, color)
STATIC mp_obj_t framebuffer_draw_circle(size_t n_args, const mp_obj_t * args) {
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(args[0]);
  mp_int_t cx = mp_obj_get_int(args[1]);
  mp_int_t cy = mp_obj_get_int(args[2]);
  mp_int_t r = mp_obj_get_int(args[3]);
  if (r < 0) r = -r;
  (void)cx; (void)cy;
  KDColor color = parse_color_or_palette(self, args[4]);
  init_rgb222_table();
  int w = self->size.width();
  int h = self->size.height();
  int minx = cx - r; if (minx < 0) minx = 0;
  int miny = cy - r; if (miny < 0) miny = 0;
  int maxx = cx + r; if (maxx >= w) maxx = w-1;
  int maxy = cy + r; if (maxy >= h) maxy = h-1;
  if (minx > maxx || miny > maxy) return mp_const_none;
  int rectw = maxx - minx + 1;
  int recth = maxy - miny + 1;
  KDColor * tmp = static_cast<KDColor *>(m_malloc(rectw * recth * sizeof(KDColor)));
  if (!tmp) mp_raise_msg(&mp_type_MemoryError, "not enough memory for draw buffer");
  for (int j = 0; j < recth; j++) {
    for (int i = 0; i < rectw; i++) {
      size_t idx = (size_t)(miny + j) * w + (minx + i);
      uint32_t pix = packed_get_pixel_bits(self->pixels, idx, self->bitsPerPixel);
      if (self->format == 1) tmp[j*rectw + i] = rgb222_to_kdcolor[pix];
      else if (self->format == 2) tmp[j*rectw + i] = rgb332_to_kdcolor[pix];
      else if (self->format == 3) tmp[j*rectw + i] = KDColor::RGB16((uint16_t)pix);
      else tmp[j*rectw + i] = (pix < self->palette_len) ? self->palette[pix] : KDColor::RGB16(0);
    }
  }
  KDFrameBuffer fb(tmp, KDSize(rectw, recth));
  KDFrameBufferContext ctx(&fb);
  ctx.drawCircle(KDPoint(cx - minx, cy - miny), r, color);
  for (int j = 0; j < recth; j++) {
    for (int i = 0; i < rectw; i++) {
      size_t idx = (size_t)(miny + j) * w + (minx + i);
      uint32_t idxColor;
      if (self->format == 1) idxColor = (uint32_t)kdcolor_to_rgb222(tmp[j*rectw + i]);
      else if (self->format == 2) idxColor = (uint32_t)(((tmp[j*rectw + i].red()>>5)<<5)|((tmp[j*rectw + i].green()>>5)<<2)|(tmp[j*rectw + i].blue()>>6));
      else if (self->format == 3) idxColor = (uint32_t)(((tmp[j*rectw + i].red()>>3)<<11)|((tmp[j*rectw + i].green()>>2)<<5)|(tmp[j*rectw + i].blue()>>3));
      else {
        idxColor = (uint32_t)palette_find_closest_index(self, tmp[j*rectw + i]);
      }
      packed_set_pixel_bits(self->pixels, idx, idxColor, self->bitsPerPixel);
    }
  }
  m_free(tmp);
  return mp_const_none;
}
const mp_obj_fun_builtin_var_t framebuffer_draw_circle_obj = {
  {&mp_type_fun_builtin_var},
  MP_OBJ_FUN_MAKE_SIG(5,5,false),
  {(mp_fun_var_t)framebuffer_draw_circle}
};

// fill_circle(self, x, y, r, color)
STATIC mp_obj_t framebuffer_fill_circle(size_t n_args, const mp_obj_t * args) {
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(args[0]);
  mp_int_t cx = mp_obj_get_int(args[1]);
  mp_int_t cy = mp_obj_get_int(args[2]);
  mp_int_t r = mp_obj_get_int(args[3]);
  if (r < 0) r = -r;
  (void)cx; (void)cy;
  KDColor color = parse_color_or_palette(self, args[4]);
  init_rgb222_table();
  init_rgb332_table();
  int w = self->size.width();
  int h = self->size.height();
  int minx = cx - r; if (minx < 0) minx = 0;
  int miny = cy - r; if (miny < 0) miny = 0;
  int maxx = cx + r; if (maxx >= w) maxx = w-1;
  int maxy = cy + r; if (maxy >= h) maxy = h-1;
  if (minx > maxx || miny > maxy) return mp_const_none;
  int rectw = maxx - minx + 1;
  int recth = maxy - miny + 1;
  KDColor * tmp = static_cast<KDColor *>(m_malloc(rectw * recth * sizeof(KDColor)));
  if (!tmp) mp_raise_msg(&mp_type_MemoryError, "not enough memory for draw buffer");
  for (int j = 0; j < recth; j++) {
    for (int i = 0; i < rectw; i++) {
      size_t idx = (size_t)(miny + j) * w + (minx + i);
      uint32_t pix = packed_get_pixel_bits(self->pixels, idx, self->bitsPerPixel);
      if (self->format == 1) tmp[j*rectw + i] = rgb222_to_kdcolor[pix];
      else if (self->format == 2) tmp[j*rectw + i] = rgb332_to_kdcolor[pix];
      else if (self->format == 3) tmp[j*rectw + i] = KDColor::RGB16((uint16_t)pix);
      else tmp[j*rectw + i] = (pix < self->palette_len) ? self->palette[pix] : KDColor::RGB16(0);
    }
  }
  KDFrameBuffer fb(tmp, KDSize(rectw, recth));
  KDFrameBufferContext ctx(&fb);
  ctx.fillCircle(KDPoint(cx - minx, cy - miny), r, color);
  for (int j = 0; j < recth; j++) {
    for (int i = 0; i < rectw; i++) {
      size_t idx = (size_t)(miny + j) * w + (minx + i);
      uint32_t idxColor;
      if (self->format == 1) idxColor = (uint32_t)kdcolor_to_rgb222(tmp[j*rectw + i]);
      else if (self->format == 2) idxColor = (uint32_t)(((tmp[j*rectw + i].red()>>5)<<5)|((tmp[j*rectw + i].green()>>5)<<2)|(tmp[j*rectw + i].blue()>>6));
      else if (self->format == 3) idxColor = (uint32_t)(((tmp[j*rectw + i].red()>>3)<<11)|((tmp[j*rectw + i].green()>>2)<<5)|(tmp[j*rectw + i].blue()>>3));
      else {
        idxColor = (uint32_t)palette_find_closest_index(self, tmp[j*rectw + i]);
      }
      packed_set_pixel_bits(self->pixels, idx, idxColor, self->bitsPerPixel);
    }
  }
  m_free(tmp);
  return mp_const_none;
}
const mp_obj_fun_builtin_var_t framebuffer_fill_circle_obj = {
  {&mp_type_fun_builtin_var},
  MP_OBJ_FUN_MAKE_SIG(5,5,false),
  {(mp_fun_var_t)framebuffer_fill_circle}
};

// fill_rect(self, x, y, w, h, color)
STATIC mp_obj_t framebuffer_fill_rect(size_t n_args, const mp_obj_t * args) {
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(args[0]);
  mp_int_t x = mp_obj_get_int(args[1]);
  mp_int_t y = mp_obj_get_int(args[2]);
  mp_int_t w = mp_obj_get_int(args[3]);
  mp_int_t h = mp_obj_get_int(args[4]);
  if (w < 0) { w = -w; x = x - w; }
  if (h < 0) { h = -h; y = y - h; }
  KDRect rect(x, y, w, h);
  (void)args;
  init_rgb222_table();
  init_rgb332_table();
  int fbw = self->size.width();
  int fbh = self->size.height();
  int x0 = rect.x();
  int y0 = rect.y();
  int x1 = x0 + rect.width() - 1;
  int y1 = y0 + rect.height() - 1;
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 >= fbw) x1 = fbw - 1;
  if (y1 >= fbh) y1 = fbh - 1;
  if (x0 > x1 || y0 > y1) return mp_const_none;
  uint32_t colIndex;
  if (mp_obj_is_int(args[5])) {
    colIndex = (uint32_t)mp_obj_get_int(args[5]);
  } else {
    KDColor c = parse_color_or_palette(self, args[5]);
    if (self->format == 1) colIndex = (uint32_t)kdcolor_to_rgb222(c);
    else if (self->format == 2) colIndex = (uint32_t)(((c.red()>>5)<<5)|((c.green()>>5)<<2)|(c.blue()>>6));
    else if (self->format == 3) colIndex = (uint32_t)(((c.red()>>3)<<11)|((c.green()>>2)<<5)|(c.blue()>>3));
    else {
      bool found = false;
      for (size_t i = 0; i < self->palette_len; i++) {
        if (self->palette[i].red() == c.red() && self->palette[i].green() == c.green() && self->palette[i].blue() == c.blue()) { colIndex = (uint32_t)i; found = true; break; }
      }
      if (!found) mp_raise_ValueError("color not in palette");
    }
  }
  for (int j = y0; j <= y1; j++) {
    for (int i = 0; i <= x1 - x0; i++) {
      size_t idx = (size_t)j * fbw + (x0 + i);
      packed_set_pixel_bits(self->pixels, idx, colIndex, self->bitsPerPixel);
    }
  }
  return mp_const_none;
}
const mp_obj_fun_builtin_var_t framebuffer_fill_rect_obj = {
  {&mp_type_fun_builtin_var},
  MP_OBJ_FUN_MAKE_SIG(6,6,false),
  {(mp_fun_var_t)framebuffer_fill_rect}
};

// fill_polygon(self, points_list, color)
STATIC mp_obj_t framebuffer_fill_polygon(size_t n_args, const mp_obj_t * args) {
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(args[0]);
  size_t itemLength;
  mp_obj_t * items;
  mp_obj_get_array(args[1], &itemLength, &items);
  if (itemLength < 3) {
    mp_raise_ValueError("polygon must have at least 3 points");
  }
  KDCoordinate * pointsX = new KDCoordinate[itemLength];
  KDCoordinate * pointsY = new KDCoordinate[itemLength];
  for (size_t i = 0; i < itemLength; i++) {
    mp_obj_t * coordinates;
    mp_obj_get_array_fixed_n(items[i], 2, &coordinates);
    pointsX[i] = mp_obj_get_int(coordinates[0]);
    pointsY[i] = mp_obj_get_int(coordinates[1]);
  }
  KDColor color = parse_color_or_palette(self, args[2]);
  init_rgb222_table();
  int minx = pointsX[0], miny = pointsY[0], maxx = pointsX[0], maxy = pointsY[0];
  for (size_t i = 1; i < itemLength; i++) {
    if (pointsX[i] < minx) minx = pointsX[i];
    if (pointsX[i] > maxx) maxx = pointsX[i];
    if (pointsY[i] < miny) miny = pointsY[i];
    if (pointsY[i] > maxy) maxy = pointsY[i];
  }
  int fbw = self->size.width();
  int fbh = self->size.height();
  if (minx < 0) minx = 0;
  if (miny < 0) miny = 0;
  if (maxx >= fbw) maxx = fbw - 1;
  if (maxy >= fbh) maxy = fbh - 1;
  if (minx > maxx || miny > maxy) { delete[] pointsX; delete[] pointsY; return mp_const_none; }
  int rectw = maxx - minx + 1;
  int recth = maxy - miny + 1;
  KDColor * tmp = static_cast<KDColor *>(m_malloc(rectw * recth * sizeof(KDColor)));
  if (!tmp) { delete[] pointsX; delete[] pointsY; mp_raise_msg(&mp_type_MemoryError, "not enough memory for draw buffer"); }
  for (int j = 0; j < recth; j++) {
    for (int i = 0; i < rectw; i++) {
      size_t idx = (size_t)(miny + j) * fbw + (minx + i);
      uint32_t pix = packed_get_pixel_bits(self->pixels, idx, self->bitsPerPixel);
      if (self->format == 1) tmp[j*rectw + i] = rgb222_to_kdcolor[pix];
      else if (self->format == 2) tmp[j*rectw + i] = rgb332_to_kdcolor[pix];
      else if (self->format == 3) tmp[j*rectw + i] = KDColor::RGB16((uint16_t)pix);
      else tmp[j*rectw + i] = (pix < self->palette_len) ? self->palette[pix] : KDColor::RGB16(0);
    }
  }
  KDFrameBuffer fb(tmp, KDSize(rectw, recth));
  KDFrameBufferContext ctx(&fb);
  for (size_t i = 0; i < itemLength; i++) { pointsX[i] -= minx; pointsY[i] -= miny; }
  ctx.fillPolygon(pointsX, pointsY, itemLength, color);
  /* write back compressed */
  for (int j = 0; j < recth; j++) {
    for (int i = 0; i < rectw; i++) {
      size_t idx = (size_t)(miny + j) * fbw + (minx + i);
      uint32_t idxColor;
      if (self->format == 1) idxColor = (uint32_t)kdcolor_to_rgb222(tmp[j*rectw + i]);
      else if (self->format == 2) idxColor = (uint32_t)(((tmp[j*rectw + i].red()>>5)<<5)|((tmp[j*rectw + i].green()>>5)<<2)|(tmp[j*rectw + i].blue()>>6));
      else if (self->format == 3) idxColor = (uint32_t)(((tmp[j*rectw + i].red()>>3)<<11)|((tmp[j*rectw + i].green()>>2)<<5)|(tmp[j*rectw + i].blue()>>3));
      else {
        idxColor = (uint32_t)palette_find_closest_index(self, tmp[j*rectw + i]);
      }
      packed_set_pixel_bits(self->pixels, idx, idxColor, self->bitsPerPixel);
    }
  }
  m_free(tmp);
  delete[] pointsX;
  delete[] pointsY;
  return mp_const_none;
}
const mp_obj_fun_builtin_var_t framebuffer_fill_polygon_obj = {
  {&mp_type_fun_builtin_var},
  MP_OBJ_FUN_MAKE_SIG(3,3,false),
  {(mp_fun_var_t)framebuffer_fill_polygon}
};

// draw_on_buffer(self, dst, x, y[, pixel_size])
// Draw this framebuffer into another framebuffer while converting formats.
STATIC mp_obj_t framebuffer_draw_on_buffer(size_t n_args, const mp_obj_t *args) {
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(args[0]);
  if (!mp_obj_is_type(args[1], &kandinsky_framebuffer_type)) {
    mp_raise_TypeError("dst must be a framebuffer");
  }
  kandinsky_framebuffer_obj_t * dst = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(args[1]);
  int x = mp_obj_get_int(args[2]);
  int y = mp_obj_get_int(args[3]);
  int pixel_size = (n_args >= 5) ? mp_obj_get_int(args[4]) : 1;
  if (pixel_size <= 0) {
    mp_raise_ValueError("pixel_size must be > 0");
  }

  init_rgb222_table();
  init_rgb332_table();

  int srcW = self->size.width();
  int srcH = self->size.height();
  int dstW = dst->size.width();
  int dstH = dst->size.height();

  for (int sy = 0; sy < srcH; sy++) {
    int dy0 = y + sy * pixel_size;
    int dy1 = dy0 + pixel_size;
    if (dy1 <= 0 || dy0 >= dstH) {
      continue;
    }
    int clippedDy0 = dy0 < 0 ? 0 : dy0;
    int clippedDy1 = dy1 > dstH ? dstH : dy1;
    for (int sx = 0; sx < srcW; sx++) {
      int dx0 = x + sx * pixel_size;
      int dx1 = dx0 + pixel_size;
      if (dx1 <= 0 || dx0 >= dstW) {
        continue;
      }
      int clippedDx0 = dx0 < 0 ? 0 : dx0;
      int clippedDx1 = dx1 > dstW ? dstW : dx1;

      size_t srcIdx = (size_t)sy * (size_t)srcW + (size_t)sx;
      uint32_t srcValue = packed_get_pixel_bits(self->pixels, srcIdx, self->bitsPerPixel);
      KDColor c = framebuffer_color_from_index(self, srcValue);
      uint32_t dstValue = framebuffer_index_from_color(dst, c, false);

      for (int dy = clippedDy0; dy < clippedDy1; dy++) {
        size_t rowBase = (size_t)dy * (size_t)dstW;
        for (int dx = clippedDx0; dx < clippedDx1; dx++) {
          packed_set_pixel_bits(dst->pixels, rowBase + (size_t)dx, dstValue, dst->bitsPerPixel);
        }
      }
    }
  }

  return mp_const_none;
}
const mp_obj_fun_builtin_var_t framebuffer_draw_on_buffer_obj = {
  {&mp_type_fun_builtin_var},
  MP_OBJ_FUN_MAKE_SIG(4,5,false),
  {(mp_fun_var_t)framebuffer_draw_on_buffer}
};

// draw(self[, x, y, pixel_size])
// If x/y are omitted, draw at (0,0). pixel_size defaults to 1.
STATIC mp_obj_t framebuffer_draw(size_t n_args, const mp_obj_t *args) {
  // args: self [, x, y, pixel_size]
  kandinsky_framebuffer_obj_t * self = (kandinsky_framebuffer_obj_t*) MP_OBJ_TO_PTR(args[0]);
  int x = 0;
  int y = 0;
  int pixel_size = 1;
  if (n_args >= 2) {
    x = mp_obj_get_int(args[1]);
  }
  if (n_args >= 3) {
    y = mp_obj_get_int(args[2]);
  }
  if (n_args >= 4) {
    pixel_size = mp_obj_get_int(args[3]);
  }
  if (pixel_size <= 0) {
    mp_raise_ValueError("pixel_size must be > 0");
  }
  MicroPython::ExecutionEnvironment::currentExecutionEnvironment()->displaySandbox();
  if (pixel_size == 1) {
    // Fast path: no scaling, convert each line from RGB222 -> KDColor and blit per-line
    init_rgb222_table();
    init_rgb332_table();
    int w = self->size.width();
    int h = self->size.height();
    KDColor * lineBuf = static_cast<KDColor *>(m_malloc(w * sizeof(KDColor)));
    if (!lineBuf) {
      mp_raise_msg(&mp_type_MemoryError, "not enough memory for draw buffer");
    }
    KDContext * ctx = KDIonContext::sharedContext();
    KDPoint oldOrigin = ctx->origin();
    KDRect oldClipping = ctx->clippingRect();
    if (modkandinsky_is_fullscreen()) {
      ctx->setOrigin(KDPoint(0, 0));
      ctx->setClippingRect(KDRect(0, 0, Ion::Display::Width, Ion::Display::Height));
    }
    for (int j = 0; j < h; j++) {
      for (int i = 0; i < w; i++) {
        uint32_t v = packed_get_pixel_bits(self->pixels, (size_t)j * w + i, self->bitsPerPixel);
        if (self->format == 1) lineBuf[i] = rgb222_to_kdcolor[v];
        else if (self->format == 2) lineBuf[i] = rgb332_to_kdcolor[v];
        else if (self->format == 3) lineBuf[i] = KDColor::RGB16((uint16_t)v);
        else lineBuf[i] = (v < self->palette_len) ? self->palette[v] : KDColor::RGB16(0);
      }
      ctx->fillRectWithPixels(KDRect(KDPoint(x, y + j), KDSize(w, 1)), lineBuf, nullptr);
    }
    ctx->setOrigin(oldOrigin);
    ctx->setClippingRect(oldClipping);
    m_free(lineBuf);
    return mp_const_none;
  }
  // Scaled draw: draw each pixel as a filled rect
  init_rgb222_table();
  init_rgb332_table();
  KDContext * ctx = KDIonContext::sharedContext();
  KDPoint oldOrigin = ctx->origin();
  KDRect oldClipping = ctx->clippingRect();
  if (modkandinsky_is_fullscreen()) {
    ctx->setOrigin(KDPoint(0, 0));
    ctx->setClippingRect(KDRect(0, 0, Ion::Display::Width, Ion::Display::Height));
  }
  for (int j = 0; j < self->size.height(); j++) {
    for (int i = 0; i < self->size.width(); i++) {
      uint32_t v = packed_get_pixel_bits(self->pixels, (size_t)j * self->size.width() + i, self->bitsPerPixel);
      KDColor c;
      if (self->format == 1) c = rgb222_to_kdcolor[v];
      else if (self->format == 2) c = rgb332_to_kdcolor[v];
      else if (self->format == 3) c = KDColor::RGB16((uint16_t)v);
      else c = (v < self->palette_len) ? self->palette[v] : KDColor::RGB16(0);
      KDRect dest(x + i*pixel_size, y + j*pixel_size, pixel_size, pixel_size);
      ctx->fillRect(dest, c);
    }
  }
  ctx->setOrigin(oldOrigin);
  ctx->setClippingRect(oldClipping);
  return mp_const_none;
}
const mp_obj_fun_builtin_var_t framebuffer_draw_obj = {
  {&mp_type_fun_builtin_var},
  MP_OBJ_FUN_MAKE_SIG(1, 4, false),
  {(mp_fun_var_t)framebuffer_draw}
};

STATIC void framebuffer_attr(mp_obj_t self_in, qstr attribute, mp_obj_t *destination) {
  // destination[0] = value, destination[1] = self for bound methods
  switch(attribute) {
    case MP_QSTR_fill:
      destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_fill_obj);
      destination[1] = self_in;
      break;
    /* update removed */
    case MP_QSTR_set_pixel:
      destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_set_pixel_obj);
      destination[1] = self_in;
      break;
    case MP_QSTR_get_pixel:
      destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_get_pixel_obj);
      destination[1] = self_in;
      break;
    case MP_QSTR_get_pixels:
      destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_get_pixels_obj);
      destination[1] = self_in;
      break;
    case MP_QSTR_draw:
      destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_draw_obj);
      destination[1] = self_in;
      break;
    case MP_QSTR_draw_line:
      destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_draw_line_obj);
      destination[1] = self_in;
      break;
    case MP_QSTR_draw_string:
      destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_draw_string_obj);
      destination[1] = self_in;
      break;
    case MP_QSTR_draw_circle:
      destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_draw_circle_obj);
      destination[1] = self_in;
      break;
    case MP_QSTR_fill_circle:
      destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_fill_circle_obj);
      destination[1] = self_in;
      break;
    case MP_QSTR_fill_rect:
      destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_fill_rect_obj);
      destination[1] = self_in;
      break;
    case MP_QSTR_fill_polygon:
      destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_fill_polygon_obj);
      destination[1] = self_in;
      break;
    case MP_QSTR_close:
      destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_close_obj);
      destination[1] = self_in;
      break;
    case MP_QSTR_width:
      destination[0] = mp_obj_new_int(self_in ? ((kandinsky_framebuffer_obj_t*)MP_OBJ_TO_PTR(self_in))->size.width() : 0);
      break;
    case MP_QSTR_height:
      destination[0] = mp_obj_new_int(self_in ? ((kandinsky_framebuffer_obj_t*)MP_OBJ_TO_PTR(self_in))->size.height() : 0);
      break;
    default:
      break;
  }
  // provide add_color at runtime (avoid requiring MP_QSTR_add_color compile-time qstr)
  if (attribute == qstr_from_str("change_color")) {
    destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_change_color_obj);
    destination[1] = self_in;
  }
  
  if (attribute == qstr_from_str("get_palette_entry")) {
    destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_get_palette_entry_obj);
    destination[1] = self_in;
  }
  if (attribute == qstr_from_str("get_raw_index")) {
    destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_get_raw_index_obj);
    destination[1] = self_in;
  }
  if (attribute == qstr_from_str("draw_on_buffer")) {
    destination[0] = (mp_obj_t) MP_ROM_PTR(&framebuffer_draw_on_buffer_obj);
    destination[1] = self_in;
  }
}

STATIC const mp_rom_map_elem_t framebuffer_locals_table[] = {
  { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&framebuffer_fill_obj) },
  { MP_ROM_QSTR(MP_QSTR_set_pixel), MP_ROM_PTR(&framebuffer_set_pixel_obj) },
  { MP_ROM_QSTR(MP_QSTR_get_pixel), MP_ROM_PTR(&framebuffer_get_pixel_obj) },
  { MP_ROM_QSTR(MP_QSTR_get_pixels), MP_ROM_PTR(&framebuffer_get_pixels_obj) },
  { MP_ROM_QSTR(MP_QSTR_draw), MP_ROM_PTR(&framebuffer_draw_obj) },
  { MP_ROM_QSTR(MP_QSTR_draw_line), MP_ROM_PTR(&framebuffer_draw_line_obj) },
  { MP_ROM_QSTR(MP_QSTR_draw_string), MP_ROM_PTR(&framebuffer_draw_string_obj) },
  { MP_ROM_QSTR(MP_QSTR_draw_circle), MP_ROM_PTR(&framebuffer_draw_circle_obj) },
  { MP_ROM_QSTR(MP_QSTR_fill_circle), MP_ROM_PTR(&framebuffer_fill_circle_obj) },
  { MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&framebuffer_fill_rect_obj) },
  { MP_ROM_QSTR(MP_QSTR_fill_polygon), MP_ROM_PTR(&framebuffer_fill_polygon_obj) },
  { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&framebuffer_close_obj) },
};

STATIC MP_DEFINE_CONST_DICT(framebuffer_locals_dict, framebuffer_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
  kandinsky_framebuffer_type,
  MP_QSTR_framebuffer,
  0,
  make_new, (const void *)framebuffer_make_new,
  print, (const void *)framebuffer_print,
  attr, (const void *)framebuffer_attr,
  locals_dict, &framebuffer_locals_dict
);
