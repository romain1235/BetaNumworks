#ifndef PYTHON_PORT_HELPERS_H
#define PYTHON_PORT_HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// These methods return true if they have been interrupted
bool micropython_port_vm_hook_loop();
void micropython_port_vm_hook_refresh_print();
bool micropython_port_interruptible_msleep(int32_t delay);
bool micropython_port_interrupt_if_needed();
int micropython_port_random();
void micropython_port_save_led_state();
void micropython_port_restore_led_state();

#ifdef __cplusplus
}
#endif

#endif
