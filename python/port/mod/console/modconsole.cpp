extern "C" {
#include "py/runtime.h"
#include "py/obj.h"
#include "py/objtuple.h"
#include "py/objlist.h"
#include "py/mphal.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../port.h"

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

// colored_text(text, (r,g,b))
#ifdef __cplusplus
extern "C" {
#endif

mp_obj_t modconsole_colored_text(mp_obj_t text_obj, mp_obj_t color_obj) {
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

  size_t total_len = (size_t)h + textlen + 4;
  char * out = static_cast<char *>(malloc(total_len + 1));
  if (out == nullptr) {
    mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("out of memory"));
  }

  memcpy(out, header, h);
  memcpy(out + h, text, textlen);
  memcpy(out + h + textlen, "\x1b[0m", 4);
  out[total_len] = 0;

  mp_obj_t result = mp_obj_new_str(out, total_len);
  free(out);
  return result;
}

mp_obj_t modconsole_select(size_t n_args, const mp_obj_t * args) {
  if (n_args == 0) {
    mp_raise_TypeError("select expects at least one choice");
  }

  const mp_obj_t * choices = args;
  size_t choice_count = n_args;
  mp_obj_t * extracted_choices = nullptr;

  if (n_args == 1 && (mp_obj_is_type(args[0], &mp_type_list) || mp_obj_is_type(args[0], &mp_type_tuple))) {
    mp_obj_get_array(args[0], &choice_count, &extracted_choices);
    choices = extracted_choices;
  }

  if (choice_count == 0) {
    mp_raise_TypeError("select choices cannot be empty");
  }

  const char ** utf8_choices = static_cast<const char **>(malloc(choice_count * sizeof(const char *)));
  if (utf8_choices == nullptr) {
    mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("out of memory"));
  }

  for (size_t i = 0; i < choice_count; i++) {
    utf8_choices[i] = mp_obj_str_get_str(choices[i]);
  }

  MicroPython::ExecutionEnvironment * env = MicroPython::ExecutionEnvironment::currentExecutionEnvironment();
  if (env == nullptr) {
    free(utf8_choices);
    mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("select requires running code context"));
  }

  int selected_index = env->selectText(utf8_choices, choice_count);
  free(utf8_choices);
  if (selected_index < 0) {
    mp_raise_type(&mp_type_KeyboardInterrupt);
  }
  return mp_obj_new_int(selected_index);
}
#ifdef __cplusplus
}
#endif

/* End of implementations. Module function objects and table are defined in
  modconsole_table.c to allow using C designated initializers. */
