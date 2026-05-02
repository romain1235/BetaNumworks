extern "C" {
#include "modescher.h"
#include <py/obj.h>
#include <py/runtime.h>
}

#include <escher/clipboard.h>
#include "port.h"
#include <string.h>

mp_obj_t modescher_get_clipboard() {
  const char *text = Clipboard::sharedClipboard()->storedText();
  if (text == nullptr) {
    return mp_const_none;
  }
  micropython_port_interrupt_if_needed();
  return mp_obj_new_str(text, strlen(text));
}

mp_obj_t modescher_set_clipboard(mp_obj_t o_text) {
  size_t len;
  const char *text = mp_obj_str_get_data(o_text, &len);
  Clipboard::sharedClipboard()->store(text, (int)len);
  micropython_port_interrupt_if_needed();
  return mp_const_none;
}
