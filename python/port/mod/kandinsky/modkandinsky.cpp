extern "C" {
#include "modkandinsky.h"
#include <py/runtime.h>
}
#include <escher/palette.h>
#include <escher/metric.h>
#include <kandinsky.h>
#include <ion.h>
#include <apps/apps_container.h>
#include "port.h"
#include <py/obj.h>


static mp_obj_t TupleForKDColor(KDColor c) {
  mp_obj_tuple_t * t = static_cast<mp_obj_tuple_t *>(MP_OBJ_TO_PTR(mp_obj_new_tuple(3, NULL)));
  t->items[0] = MP_OBJ_NEW_SMALL_INT(c.red());
  t->items[1] = MP_OBJ_NEW_SMALL_INT(c.green());
  t->items[2] = MP_OBJ_NEW_SMALL_INT(c.blue());
  return MP_OBJ_FROM_PTR(t);
}

// Fullscreen flag: when true, kandinsky drawing primitives use the full
// display area (0,0,Width,Height) as clipping/origin.
static bool s_kandinsky_fullscreen = false;

// User clip in sandbox-local coordinates. It is intersected with the base clip
// (sandbox area or fullscreen area).
static bool s_kandinsky_clip_enabled = false;
static KDRect s_kandinsky_clip_rect = KDRectZero;

static KDCoordinate kandinskyTitleBarHeight() {
  return s_kandinsky_fullscreen ? 0 : Metric::TitleBarHeight;
}

static KDRect kandinskyBaseClipRect() {
  KDCoordinate titleHeight = kandinskyTitleBarHeight();
  if (s_kandinsky_fullscreen) {
    return KDRect(0, 0, Ion::Display::Width, Ion::Display::Height);
  }
  return KDRect(0, titleHeight, Ion::Display::Width, Ion::Display::Height - titleHeight);
}

static KDRect kandinskyEffectiveClipRect() {
  KDRect clip = kandinskyBaseClipRect();
  if (s_kandinsky_clip_enabled) {
    clip = clip.intersectedWith(s_kandinsky_clip_rect.translatedBy(KDPoint(0, kandinskyTitleBarHeight())));
  }
  return clip;
}

void MicroPython::Kandinsky::ApplyDrawingContext(KDContext * ctx) {
  ctx->setOrigin(KDPoint(0, kandinskyTitleBarHeight()));
  ctx->setClippingRect(kandinskyEffectiveClipRect());
}

static void modkandinsky_reset_clip_state() {
  s_kandinsky_clip_enabled = false;
  s_kandinsky_clip_rect = KDRectZero;
}


/* KDIonContext::sharedContext needs to be set to the wanted Rect before
 * calling kandinsky_get_pixel, kandinsky_set_pixel and kandinsky_draw_string.
 * We do this here with displaySandbox(), which pushes the SandboxController on
 * the stackViewController and forces the window to redraw itself.
 * KDIonContext::sharedContext is set to the frame of the last object drawn. */

mp_obj_t modkandinsky_color(size_t n_args, const mp_obj_t *args) {
  mp_obj_t color;
  if (n_args == 1) {
    color = args[0];
  } else if (n_args == 2) {
    mp_raise_TypeError("color takes 1 or 3 arguments");
    return mp_const_none;
  } else {
    assert(n_args == 3);
    color = mp_obj_new_tuple(n_args, args);
  }
  return TupleForKDColor(MicroPython::Color::Parse(color));
}

/* Calling ExecutionEnvironment::displaySandbox() hides the console and switches
 * to another mode. So it's a good idea to retrieve and handle input parameters
 * before calling displaySandbox, otherwise error messages (such as TypeError)
 * won't be visible until the user comes back to the console screen. */

mp_obj_t modkandinsky_get_pixel(mp_obj_t x, mp_obj_t y) {
  KDPoint point(mp_obj_get_int(x), mp_obj_get_int(y));
  KDColor c;
  KDContext * ctx = KDIonContext::sharedContext();
  KDPoint oldOrigin = ctx->origin();
  KDRect oldClipping = ctx->clippingRect();
  MicroPython::Kandinsky::ApplyDrawingContext(ctx);
  ctx->getPixel(point, &c);
  ctx->setOrigin(oldOrigin);
  ctx->setClippingRect(oldClipping);
  return TupleForKDColor(c);
}

mp_obj_t modkandinsky_get_pixels(size_t n_args, const mp_obj_t *args) {
  if (n_args != 4) {
    mp_raise_TypeError("get_pixels expects 4 arguments: x,y,w,h");
  }
  int x = mp_obj_get_int(args[0]);
  int y = mp_obj_get_int(args[1]);
  int w = mp_obj_get_int(args[2]);
  int h = mp_obj_get_int(args[3]);
  if (w <= 0 || h <= 0) {
    mp_raise_ValueError("width and height must be > 0");
  }
  KDRect rect(x, y, w, h);
  KDColor * buf = static_cast<KDColor *>(m_malloc((size_t)w * (size_t)h * sizeof(KDColor)));
  if (!buf) mp_raise_msg(&mp_type_MemoryError, "not enough memory for pixels");
  KDContext * ctx = KDIonContext::sharedContext();
  KDPoint oldOrigin = ctx->origin();
  KDRect oldClipping = ctx->clippingRect();
  MicroPython::Kandinsky::ApplyDrawingContext(ctx);
  ctx->getPixels(rect, buf);
  ctx->setOrigin(oldOrigin);
  ctx->setClippingRect(oldClipping);

  mp_obj_t outer = mp_obj_new_list(0, NULL);
  for (int j = 0; j < h; j++) {
    mp_obj_t row = mp_obj_new_list(0, NULL);
    for (int i = 0; i < w; i++) {
      mp_obj_t t = TupleForKDColor(buf[j * w + i]);
      mp_obj_list_append(row, t);
    }
    mp_obj_list_append(outer, row);
  }
  m_free(buf);
  return outer;
}

mp_obj_t modkandinsky_set_pixel(mp_obj_t x, mp_obj_t y, mp_obj_t input) {
  KDPoint point(mp_obj_get_int(x), mp_obj_get_int(y));
  KDColor kdColor = MicroPython::Color::Parse(input);
  MicroPython::ExecutionEnvironment::currentExecutionEnvironment()->displaySandbox();
  KDContext * ctx = KDIonContext::sharedContext();
  KDPoint oldOrigin = ctx->origin();
  KDRect oldClipping = ctx->clippingRect();
  MicroPython::Kandinsky::ApplyDrawingContext(ctx);
  ctx->setPixel(point, kdColor);
  ctx->setOrigin(oldOrigin);
  ctx->setClippingRect(oldClipping);
  return mp_const_none;
}

/* Font ids used by kandinsky.draw_string:
 *   0 -> LargeFont        4 -> ItalicLargeFont
 *   1 -> SmallFont        5 -> ItalicSmallFont
 *   3 -> TinyFont */
const KDFont * MicroPython::Kandinsky::FontForId(int id) {
  switch (id) {
    case 1:
      return KDFont::SmallFont;
    case 3:
      return KDFont::TinyFont;
    case 4:
      return KDFont::ItalicLargeFont;
    case 5:
      return KDFont::ItalicSmallFont;
    case 0:
    default:
      return KDFont::LargeFont;
  }
}

mp_obj_t modkandinsky_draw_string(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
  enum {
    ARG_text,
    ARG_x,
    ARG_y,
    ARG_color,
    ARG_background,
    ARG_font,
    ARG_italic,
  };
  static const mp_arg_t allowedArgs[] = {
    { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_color, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    { MP_QSTR_background, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    { MP_QSTR_font, MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_italic, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
  };
  mp_arg_val_t args[MP_ARRAY_SIZE(allowedArgs)];
  mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowedArgs), allowedArgs, args);

  const char * text = mp_obj_str_get_str(args[ARG_text].u_obj);
  KDPoint point(args[ARG_x].u_int, args[ARG_y].u_int);
  KDColor textColor = (args[ARG_color].u_obj != mp_const_none)
    ? MicroPython::Color::Parse(args[ARG_color].u_obj) : Palette::PrimaryText;
  KDColor backgroundColor = (args[ARG_background].u_obj != mp_const_none)
    ? MicroPython::Color::Parse(args[ARG_background].u_obj) : Palette::HomeBackground;
  const KDFont * font = MicroPython::Kandinsky::FontForId(args[ARG_font].u_int);
  // Legacy 7th positional argument: force the italic variant of the font.
  if (args[ARG_italic].u_obj != mp_const_none && mp_obj_is_true(args[ARG_italic].u_obj)) {
    font = font->toItalic();
  }
  MicroPython::ExecutionEnvironment::currentExecutionEnvironment()->displaySandbox();
  KDContext * ctx = KDIonContext::sharedContext();
  KDPoint oldOrigin = ctx->origin();
  KDRect oldClipping = ctx->clippingRect();
  MicroPython::Kandinsky::ApplyDrawingContext(ctx);
  ctx->drawString(text, point, font, textColor, backgroundColor);
  ctx->setOrigin(oldOrigin);
  ctx->setClippingRect(oldClipping);
  return mp_const_none;
}

mp_obj_t modkandinsky_draw_line(size_t n_args, const mp_obj_t * args) {
  mp_int_t x1 = mp_obj_get_int(args[0]);
  mp_int_t y1 = mp_obj_get_int(args[1]);
  mp_int_t x2 = mp_obj_get_int(args[2]);
  mp_int_t y2 = mp_obj_get_int(args[3]);
  KDPoint p1 = KDPoint(x1, y1);
  KDPoint p2 = KDPoint(x2, y2);
  KDColor color = MicroPython::Color::Parse(args[4]);
  MicroPython::ExecutionEnvironment::currentExecutionEnvironment()->displaySandbox();
  KDContext * ctx = KDIonContext::sharedContext();
  KDPoint oldOrigin = ctx->origin();
  KDRect oldClipping = ctx->clippingRect();
  MicroPython::Kandinsky::ApplyDrawingContext(ctx);
  ctx->drawLine(p1, p2, color);
  ctx->setOrigin(oldOrigin);
  ctx->setClippingRect(oldClipping);
  return mp_const_none;
}

mp_obj_t modkandinsky_draw_circle(size_t n_args, const mp_obj_t * args) {
  mp_int_t cx = mp_obj_get_int(args[0]);
  mp_int_t cy = mp_obj_get_int(args[1]);
  mp_int_t r = mp_obj_get_int(args[2]);
  if(r<0)
  {
    r = -r;
  }
  KDPoint center = KDPoint(cx, cy);
  KDColor color = MicroPython::Color::Parse(args[3]);
  MicroPython::ExecutionEnvironment::currentExecutionEnvironment()->displaySandbox();
  KDContext * ctx = KDIonContext::sharedContext();
  KDPoint oldOrigin = ctx->origin();
  KDRect oldClipping = ctx->clippingRect();
  MicroPython::Kandinsky::ApplyDrawingContext(ctx);
  ctx->drawCircle(center, r, color);
  ctx->setOrigin(oldOrigin);
  ctx->setClippingRect(oldClipping);
  return mp_const_none;
}

mp_obj_t modkandinsky_fill_rect(size_t n_args, const mp_obj_t * args) {
  mp_int_t x = mp_obj_get_int(args[0]);
  mp_int_t y = mp_obj_get_int(args[1]);
  mp_int_t width = mp_obj_get_int(args[2]);
  mp_int_t height = mp_obj_get_int(args[3]);
  if (width < 0) {
    width = -width;
    x = x - width;
  }
  if (height < 0) {
    height = -height;
    y = y - height;
  }
  KDRect rect(x, y, width, height);
  KDColor color = MicroPython::Color::Parse(args[4]);
  MicroPython::ExecutionEnvironment::currentExecutionEnvironment()->displaySandbox();
  KDContext * ctx = KDIonContext::sharedContext();
  KDPoint oldOrigin = ctx->origin();
  KDRect oldClipping = ctx->clippingRect();
  MicroPython::Kandinsky::ApplyDrawingContext(ctx);
  ctx->fillRect(rect, color);
  ctx->setOrigin(oldOrigin);
  ctx->setClippingRect(oldClipping);
  return mp_const_none;
}

mp_obj_t modkandinsky_fill_circle(size_t n_args, const mp_obj_t * args) {
  mp_int_t cx = mp_obj_get_int(args[0]);
  mp_int_t cy = mp_obj_get_int(args[1]);
  mp_int_t r = mp_obj_get_int(args[2]);
  if(r<0)
  {
    r = -r;
  }
  KDPoint center = KDPoint(cx, cy);
  KDColor color = MicroPython::Color::Parse(args[3]);
  MicroPython::ExecutionEnvironment::currentExecutionEnvironment()->displaySandbox();
  KDContext * ctx = KDIonContext::sharedContext();
  KDPoint oldOrigin = ctx->origin();
  KDRect oldClipping = ctx->clippingRect();
  MicroPython::Kandinsky::ApplyDrawingContext(ctx);
  ctx->fillCircle(center, r, color);
  ctx->setOrigin(oldOrigin);
  ctx->setClippingRect(oldClipping);
  return mp_const_none;
}

mp_obj_t modkandinsky_fill_polygon(size_t n_args, const mp_obj_t * args) {
  size_t itemLength;
  mp_obj_t * items;

  mp_obj_get_array(args[0], &itemLength, &items);

  KDCoordinate pointsX[itemLength];
  KDCoordinate pointsY[itemLength];

  if (itemLength < 3) {
    mp_raise_ValueError("polygon must have at least 3 points");
  }

  for(int i=0; i<itemLength; i++) {
    mp_obj_t * coordinates;
    mp_obj_get_array_fixed_n(items[i], 2, &coordinates);

    pointsX[i] = mp_obj_get_int(coordinates[0]);
    pointsY[i] = mp_obj_get_int(coordinates[1]);
  }

  KDColor color = MicroPython::Color::Parse(args[1]);
  MicroPython::ExecutionEnvironment::currentExecutionEnvironment()->displaySandbox();
  KDContext * ctx = KDIonContext::sharedContext();
  KDPoint oldOrigin = ctx->origin();
  KDRect oldClipping = ctx->clippingRect();
  MicroPython::Kandinsky::ApplyDrawingContext(ctx);
  ctx->fillPolygon(pointsX, pointsY, itemLength, color);
  ctx->setOrigin(oldOrigin);
  ctx->setClippingRect(oldClipping);
  return mp_const_none;
} 

mp_obj_t modkandinsky_wait_vblank() {
  micropython_port_interrupt_if_needed();
  Ion::Display::waitForVBlank();
  return mp_const_none;
}

mp_obj_t modkandinsky_set_fullscreen(mp_obj_t enable_obj) {
  bool enable = mp_obj_is_true(enable_obj);
  s_kandinsky_fullscreen = enable;
  if (!enable) {
    AppsContainer::sharedAppsContainer()->redrawWindow(true);
  }
  return mp_const_none;
}

mp_obj_t modkandinsky_get_fullscreen() {
  return s_kandinsky_fullscreen ? mp_const_true : mp_const_false;
}

mp_obj_t modkandinsky_set_clip(size_t n_args, const mp_obj_t *args) {
  if (n_args != 4) {
    mp_raise_TypeError("set_clip expects 4 arguments: x,y,w,h");
  }
  int x = mp_obj_get_int(args[0]);
  int y = mp_obj_get_int(args[1]);
  int w = mp_obj_get_int(args[2]);
  int h = mp_obj_get_int(args[3]);
  if (w <= 0 || h <= 0) {
    mp_raise_ValueError("width and height must be > 0");
  }
  s_kandinsky_clip_rect = KDRect(x, y, w, h);
  s_kandinsky_clip_enabled = true;
  return mp_const_none;
}

mp_obj_t modkandinsky_reset_clip() {
  modkandinsky_reset_clip_state();
  return mp_const_none;
}

mp_obj_t modkandinsky_get_clip() {
  KDRect clip = kandinskyEffectiveClipRect();
  KDCoordinate titleHeight = kandinskyTitleBarHeight();
  mp_obj_tuple_t * t = static_cast<mp_obj_tuple_t *>(MP_OBJ_TO_PTR(mp_obj_new_tuple(4, NULL)));
  t->items[0] = MP_OBJ_NEW_SMALL_INT(clip.x());
  t->items[1] = MP_OBJ_NEW_SMALL_INT(clip.y() - titleHeight);
  t->items[2] = MP_OBJ_NEW_SMALL_INT(clip.width());
  t->items[3] = MP_OBJ_NEW_SMALL_INT(clip.height());
  return MP_OBJ_FROM_PTR(t);
}

extern "C" bool modkandinsky_is_fullscreen(void) {
  return s_kandinsky_fullscreen;
}

void modkandinsky_reset_fullscreen(void) {
  if (s_kandinsky_fullscreen) {
    s_kandinsky_fullscreen = false;
    AppsContainer::sharedAppsContainer()->redrawWindow(true);
  }
}

void modkandinsky_view_did_disappear(void) {
  // When the Kandinsky display is closed, ensure fullscreen is cleared
  modkandinsky_reset_fullscreen();
  modkandinsky_reset_clip_state();
}

mp_obj_t modkandinsky_get_palette() {
  mp_obj_t modkandinsky_palette_table = mp_obj_new_dict(0);
  mp_obj_dict_store(modkandinsky_palette_table, MP_ROM_QSTR(MP_QSTR_PrimaryText), TupleForKDColor(Palette::PrimaryText));
  mp_obj_dict_store(modkandinsky_palette_table, MP_ROM_QSTR(MP_QSTR_SecondaryText), TupleForKDColor(Palette::SecondaryText));
  mp_obj_dict_store(modkandinsky_palette_table, MP_ROM_QSTR(MP_QSTR_AccentText), TupleForKDColor(Palette::AccentText));
  mp_obj_dict_store(modkandinsky_palette_table, MP_ROM_QSTR(MP_QSTR_Toolbar), TupleForKDColor(Palette::Toolbar));
  mp_obj_dict_store(modkandinsky_palette_table, MP_ROM_QSTR(MP_QSTR_HomeBackground), TupleForKDColor(Palette::HomeBackground));

  return modkandinsky_palette_table;
}

