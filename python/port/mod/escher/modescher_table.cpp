extern "C" {
#include "modescher.h"
#include <py/obj.h>
#include <py/runtime.h>
}

extern "C" const mp_obj_fun_builtin_fixed_t modescher_get_clipboard_obj = {
  {&mp_type_fun_builtin_0},
  {(mp_fun_0_t)modescher_get_clipboard}
};

extern "C" const mp_obj_fun_builtin_fixed_t modescher_set_clipboard_obj = {
  {&mp_type_fun_builtin_1},
  {(mp_fun_0_t)modescher_set_clipboard}
};

extern "C" const mp_rom_map_elem_t modescher_module_globals_table[] = {
  { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_escher) },
  { MP_ROM_QSTR(MP_QSTR_get_clipboard), MP_ROM_PTR(&modescher_get_clipboard_obj) },
  { MP_ROM_QSTR(MP_QSTR_set_clipboard), MP_ROM_PTR(&modescher_set_clipboard_obj) },
};

STATIC MP_DEFINE_CONST_DICT(modescher_module_globals, modescher_module_globals_table);

extern "C" const mp_obj_module_t modescher_module = {
  { &mp_type_module },
  (mp_obj_dict_t*)&modescher_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_escher, modescher_module);
