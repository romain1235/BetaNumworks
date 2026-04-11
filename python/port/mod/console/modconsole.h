#ifndef PY_MOD_CONSOLE_H
#define PY_MOD_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif
#include "py/obj.h"
#ifdef __cplusplus
}
#endif

// Declarations of the functions implemented in modconsole.cpp
#ifdef __cplusplus
extern "C" {
#endif
mp_obj_t modconsole_print_color(mp_obj_t text_obj, mp_obj_t color_obj);
mp_obj_t modconsole_print_color_list(mp_obj_t list_obj);
#ifdef __cplusplus
}
#endif

#endif
