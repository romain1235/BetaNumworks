#include <py/obj.h>

mp_obj_t modion_keyboard_keydown(mp_obj_t key_o);
mp_obj_t modion_battery();
mp_obj_t modion_battery_level();
mp_obj_t modion_battery_ischarging();
mp_obj_t modion_get_keys();
mp_obj_t modion_set_brightness(mp_obj_t brightness_mp);
mp_obj_t modion_get_brightness();
extern const mp_obj_type_t file_type;

// LED control
mp_obj_t modion_led_on();
mp_obj_t modion_led_off();
mp_obj_t modion_led_color(size_t n_args, const mp_obj_t *args);
