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
mp_obj_t modconsole_colored_text(mp_obj_t text_obj, mp_obj_t color_obj);
mp_obj_t modconsole_select(size_t n_args, const mp_obj_t * args);
#ifdef __cplusplus
}
#endif

#endif
