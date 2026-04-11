extern "C" {
#include "py/runtime.h"
#include "py/obj.h"
#include "py/objtuple.h"
#include "py/objlist.h"
#include "py/mphal.h"
}

#include <stdio.h>
#include <string.h>

// Helper to append an unsigned integer in base 10 to a buffer at position pos.
static int append_uint_to_buf(char * buf, int pos, unsigned v) {
  char tmp[12];
  int tp = 0;
  if (v == 0) {
    tmp[tp++] = '0';
  } else {
    while (v > 0 && tp < (int)sizeof(tmp)) {
      tmp[tp++] = '0' + (v % 10);
      v /= 10;
    }
  }
  for (int i = tp - 1; i >= 0; i--) {
    buf[pos++] = tmp[i];
  }
  return pos;
}

// print_color(text, (r,g,b))
#ifdef __cplusplus
extern "C" {
#endif

mp_obj_t modconsole_print_color(mp_obj_t text_obj, mp_obj_t color_obj) {
  const char * text = mp_obj_str_get_str(text_obj);
  mp_obj_t * items;
  mp_obj_get_array_fixed_n(color_obj, 3, &items);
  int r = mp_obj_get_int(items[0]);
  int g = mp_obj_get_int(items[1]);
  int b = mp_obj_get_int(items[2]);

  char header[32];
  int h = 0;
  header[h++] = '\x1b';
  header[h++] = '[';
  header[h++] = 'C';
  h = append_uint_to_buf(header, h, (unsigned)r);
  header[h++] = ',';
  h = append_uint_to_buf(header, h, (unsigned)g);
  header[h++] = ',';
  h = append_uint_to_buf(header, h, (unsigned)b);
  header[h++] = ';';
  size_t textlen = strlen(text);
  mp_hal_stdout_tx_strn_cooked(header, h);
  mp_hal_stdout_tx_strn_cooked(text, textlen);
  mp_hal_stdout_tx_strn_cooked("\x1b[0m\n", 5);
  return mp_const_none;
}
// function objects and module table are defined in modconsole_table.c (C)

// print_color_list([(text,(r,g,b)), ...])
mp_obj_t modconsole_print_color_list(mp_obj_t list_obj) {
  size_t len;
  mp_obj_t * items;
  mp_obj_get_array(list_obj, &len, &items);
  for (size_t i = 0; i < len; i++) {
    mp_obj_t pair = items[i];
    mp_obj_t * pair_items;
    size_t pair_len;
    mp_obj_get_array(pair, &pair_len, &pair_items);
    if (pair_len != 2) {
      mp_raise_TypeError("each element must be (text, (r,g,b))");
    }
    const char * text = mp_obj_str_get_str(pair_items[0]);
    mp_obj_t color = pair_items[1];
    mp_obj_t * rgb;
    mp_obj_get_array_fixed_n(color, 3, &rgb);
    int r = mp_obj_get_int(rgb[0]);
    int g = mp_obj_get_int(rgb[1]);
    int b = mp_obj_get_int(rgb[2]);
    char header[32];
    int h = 0;
    header[h++] = '\x1b';
    header[h++] = '[';
    header[h++] = 'C';
    h = append_uint_to_buf(header, h, (unsigned)r);
    header[h++] = ',';
    h = append_uint_to_buf(header, h, (unsigned)g);
    header[h++] = ',';
    h = append_uint_to_buf(header, h, (unsigned)b);
    header[h++] = ';';
    mp_hal_stdout_tx_strn_cooked(header, h);
    mp_hal_stdout_tx_strn_cooked(text, strlen(text));
    // terminate each colored segment so the console parser can find the end
    // of the segment (it looks for "\x1b[0m").
    mp_hal_stdout_tx_strn_cooked("\x1b[0m", 4);
  }
  // finally emit a newline
  mp_hal_stdout_tx_strn_cooked("\n", 1);
  return mp_const_none;
}
#ifdef __cplusplus
}
#endif

/* End of implementations. Module function objects and table are defined in
  modconsole_table.c to allow using C designated initializers. */
