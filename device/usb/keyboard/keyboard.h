#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include "usb_keyboard.h"

#ifdef __cplusplus
extern "C" {
#endif

// Number of keys
#define KEYBOARD_KEYS		5

extern const uint32_t keyboard_masks[KEYBOARD_KEYS];

void keyboard_init();
uint32_t keyboard_status();

#ifdef __cplusplus
}
#endif

#endif // KEYBOARD_H
