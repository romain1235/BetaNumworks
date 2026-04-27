#include "modconsole.h"
#include "py/runtime.h"

MP_DEFINE_CONST_FUN_OBJ_2(modconsole_colored_text_obj, modconsole_colored_text);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modconsole_select_obj, 1, MP_OBJ_FUN_ARGS_MAX, modconsole_select);

STATIC const mp_rom_map_elem_t modconsole_module_globals_table[] = {
  { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_console) },
  { MP_ROM_QSTR(MP_QSTR_colored_text), MP_ROM_PTR(&modconsole_colored_text_obj) },
  { MP_ROM_QSTR(MP_QSTR_select), MP_ROM_PTR(&modconsole_select_obj) },
};

STATIC MP_DEFINE_CONST_DICT(modconsole_module_globals, modconsole_module_globals_table);

const mp_obj_module_t modconsole_module = {
  .base = { &mp_type_module },
  .globals = (mp_obj_dict_t*)&modconsole_module_globals,
};
