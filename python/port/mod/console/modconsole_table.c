#include "modconsole.h"
#include "py/runtime.h"

MP_DEFINE_CONST_FUN_OBJ_2(modconsole_print_color_obj, modconsole_print_color);
MP_DEFINE_CONST_FUN_OBJ_1(modconsole_print_color_list_obj, modconsole_print_color_list);

STATIC const mp_rom_map_elem_t modconsole_module_globals_table[] = {
  { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_console) },
  { MP_ROM_QSTR(MP_QSTR_print_color), MP_ROM_PTR(&modconsole_print_color_obj) },
  { MP_ROM_QSTR(MP_QSTR_print_color_list), MP_ROM_PTR(&modconsole_print_color_list_obj) },
};

STATIC MP_DEFINE_CONST_DICT(modconsole_module_globals, modconsole_module_globals_table);

const mp_obj_module_t modconsole_module = {
  .base = { &mp_type_module },
  .globals = (mp_obj_dict_t*)&modconsole_module_globals,
};
