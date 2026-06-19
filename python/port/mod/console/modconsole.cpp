extern "C" {
#include "py/runtime.h"
#include "py/obj.h"
#include "py/objtuple.h"
#include "py/objlist.h"
#include "py/mphal.h"
}

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

  /* Each line must be independently wrapped so splitting on '\n' keeps
   * well-formed color sequences in the console store. */
  size_t newline_count = 0;
  for (size_t i = 0; i < textlen; i++) {
    if (text[i] == '\n') {
      newline_count++;
    }
  }

  size_t total_len = (size_t)h + textlen + 4 + newline_count * ((size_t)h + 4);
  char * out = m_new(char, total_len + 1);

  size_t out_pos = 0;
  const char * p = text;
  while (true) {
    memcpy(out + out_pos, header, h);
    out_pos += h;

    const char * line_end = p;
    while (*line_end != 0 && *line_end != '\n') {
      line_end++;
    }
    size_t line_len = line_end - p;
    if (line_len > 0) {
      memcpy(out + out_pos, p, line_len);
      out_pos += line_len;
    }

    memcpy(out + out_pos, "\x1b[0m", 4);
    out_pos += 4;

    if (*line_end != '\n') {
      break;
    }

    out[out_pos++] = '\n';
    p = line_end + 1;
    if (*p == 0) {
      break;
    }
  }

  out[out_pos] = 0;

  mp_obj_t result = mp_obj_new_str(out, out_pos);
  m_free(out);
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

  const char ** utf8_choices = m_new(const char *, choice_count);

  for (size_t i = 0; i < choice_count; i++) {
    utf8_choices[i] = mp_obj_str_get_str(choices[i]);
  }

  MicroPython::ExecutionEnvironment * env = MicroPython::ExecutionEnvironment::currentExecutionEnvironment();
  if (env == nullptr) {
    m_free(utf8_choices);
    mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("select requires running code context"));
  }

  int selected_index = env->selectText(utf8_choices, choice_count);
  m_free(utf8_choices);
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
