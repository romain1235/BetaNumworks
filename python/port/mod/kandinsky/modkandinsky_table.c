#include "modkandinsky.h"

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modkandinsky_color_obj, 1, 3, modkandinsky_color);
static MP_DEFINE_CONST_FUN_OBJ_2(modkandinsky_get_pixel_obj, modkandinsky_get_pixel);
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modkandinsky_get_pixels_obj, 4, 4, modkandinsky_get_pixels);
static MP_DEFINE_CONST_FUN_OBJ_3(modkandinsky_set_pixel_obj, modkandinsky_set_pixel);
static MP_DEFINE_CONST_FUN_OBJ_KW(modkandinsky_draw_string_obj, 3, modkandinsky_draw_string);
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modkandinsky_draw_line_obj, 5, 5, modkandinsky_draw_line);
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modkandinsky_draw_circle_obj, 4, 4, modkandinsky_draw_circle);
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modkandinsky_draw_rect_obj, 5, 6, modkandinsky_draw_rect);
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modkandinsky_fill_rect_obj, 5, 5, modkandinsky_fill_rect);
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modkandinsky_fill_circle_obj, 4, 4, modkandinsky_fill_circle);
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modkandinsky_fill_polygon_obj, 2, 2, modkandinsky_fill_polygon);
static MP_DEFINE_CONST_FUN_OBJ_0(modkandinsky_wait_vblank_obj, modkandinsky_wait_vblank);
static MP_DEFINE_CONST_FUN_OBJ_0(modkandinsky_get_palette_obj, modkandinsky_get_palette);
static MP_DEFINE_CONST_FUN_OBJ_1(modkandinsky_set_fullscreen_obj, modkandinsky_set_fullscreen);
static MP_DEFINE_CONST_FUN_OBJ_0(modkandinsky_get_fullscreen_obj, modkandinsky_get_fullscreen);
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modkandinsky_set_clip_obj, 4, 4, modkandinsky_set_clip);
static MP_DEFINE_CONST_FUN_OBJ_0(modkandinsky_reset_clip_obj, modkandinsky_reset_clip);
static MP_DEFINE_CONST_FUN_OBJ_0(modkandinsky_get_clip_obj, modkandinsky_get_clip);

static const mp_rom_map_elem_t modkandinsky_module_globals_table[] = {
  { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_kandinsky) },
  { MP_ROM_QSTR(MP_QSTR_color), (mp_obj_t)&modkandinsky_color_obj },
  { MP_ROM_QSTR(MP_QSTR_get_pixel), (mp_obj_t)&modkandinsky_get_pixel_obj },
  { MP_ROM_QSTR(MP_QSTR_get_pixels), (mp_obj_t)&modkandinsky_get_pixels_obj },
  { MP_ROM_QSTR(MP_QSTR_set_pixel), (mp_obj_t)&modkandinsky_set_pixel_obj },
  { MP_ROM_QSTR(MP_QSTR_draw_string), (mp_obj_t)&modkandinsky_draw_string_obj },
  { MP_ROM_QSTR(MP_QSTR_draw_line), (mp_obj_t)&modkandinsky_draw_line_obj },
  { MP_ROM_QSTR(MP_QSTR_draw_circle), (mp_obj_t)&modkandinsky_draw_circle_obj },
  { MP_ROM_QSTR(MP_QSTR_draw_rect), (mp_obj_t)&modkandinsky_draw_rect_obj },
  { MP_ROM_QSTR(MP_QSTR_fill_rect), (mp_obj_t)&modkandinsky_fill_rect_obj },
  { MP_ROM_QSTR(MP_QSTR_fill_circle), (mp_obj_t)&modkandinsky_fill_circle_obj },
  { MP_ROM_QSTR(MP_QSTR_fill_polygon), (mp_obj_t)&modkandinsky_fill_polygon_obj },
  { MP_ROM_QSTR(MP_QSTR_LARGE_FONT), MP_ROM_INT(0) },
  { MP_ROM_QSTR(MP_QSTR_SMALL_FONT), MP_ROM_INT(1) },
  { MP_ROM_QSTR(MP_QSTR_TINY_FONT), MP_ROM_INT(3) },
  { MP_ROM_QSTR(MP_QSTR_LARGE_FONT_ITALIC), MP_ROM_INT(4) },
  { MP_ROM_QSTR(MP_QSTR_SMALL_FONT_ITALIC), MP_ROM_INT(5) },
  { MP_ROM_QSTR(MP_QSTR_wait_vblank), (mp_obj_t)&modkandinsky_wait_vblank_obj },
  { MP_ROM_QSTR(MP_QSTR_get_palette), (mp_obj_t)&modkandinsky_get_palette_obj },
  { MP_ROM_QSTR(MP_QSTR_set_fullscreen), (mp_obj_t)&modkandinsky_set_fullscreen_obj },
  { MP_ROM_QSTR(MP_QSTR_get_fullscreen), (mp_obj_t)&modkandinsky_get_fullscreen_obj },
  { MP_ROM_QSTR(MP_QSTR_set_clip), (mp_obj_t)&modkandinsky_set_clip_obj },
  { MP_ROM_QSTR(MP_QSTR_reset_clip), (mp_obj_t)&modkandinsky_reset_clip_obj },
  { MP_ROM_QSTR(MP_QSTR_get_clip), (mp_obj_t)&modkandinsky_get_clip_obj },
  { MP_ROM_QSTR(MP_QSTR_framebuffer), (mp_obj_t)&kandinsky_framebuffer_type },
};

static MP_DEFINE_CONST_DICT(modkandinsky_module_globals, modkandinsky_module_globals_table);

const mp_obj_module_t modkandinsky_module = {
  .base = { &mp_type_module },
  .globals = (mp_obj_dict_t*)&modkandinsky_module_globals,
};
