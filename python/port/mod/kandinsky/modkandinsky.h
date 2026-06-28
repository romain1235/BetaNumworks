#include <py/obj.h>

#ifdef __cplusplus
extern "C" {
#endif

mp_obj_t modkandinsky_color(size_t n_args, const mp_obj_t *args);
mp_obj_t modkandinsky_get_pixel(mp_obj_t x, mp_obj_t y);
mp_obj_t modkandinsky_set_pixel(mp_obj_t x, mp_obj_t y, mp_obj_t color);
mp_obj_t modkandinsky_draw_string(size_t n_args, const mp_obj_t *args);
mp_obj_t modkandinsky_draw_line(size_t n_args, const mp_obj_t *args);
mp_obj_t modkandinsky_draw_circle(size_t n_args, const mp_obj_t *args);
mp_obj_t modkandinsky_fill_rect(size_t n_args, const mp_obj_t *args);
mp_obj_t modkandinsky_fill_circle(size_t n_args, const mp_obj_t *args);
mp_obj_t modkandinsky_fill_polygon(size_t n_args, const mp_obj_t *args);
mp_obj_t modkandinsky_wait_vblank();
mp_obj_t modkandinsky_get_keys();
mp_obj_t modkandinsky_get_palette();
mp_obj_t modkandinsky_set_fullscreen(mp_obj_t enable);
mp_obj_t modkandinsky_get_fullscreen();
mp_obj_t modkandinsky_set_clip(size_t n_args, const mp_obj_t *args);
mp_obj_t modkandinsky_reset_clip();
mp_obj_t modkandinsky_get_clip();

mp_obj_t modkandinsky_get_pixels(size_t n_args, const mp_obj_t *args);
// Returns true when kandinsky is in fullscreen mode. C linkage for calls
// from other C/C++ parts of the codebase.
bool modkandinsky_is_fullscreen(void);

// Reset the kandinsky fullscreen flag (used by the runtime on script exit)
void modkandinsky_reset_fullscreen(void);
void modkandinsky_view_did_disappear(void);
 
// Framebuffer type (kandinsky.framebuffer)
extern const mp_obj_type_t kandinsky_framebuffer_type;

#ifdef __cplusplus
}
#endif

