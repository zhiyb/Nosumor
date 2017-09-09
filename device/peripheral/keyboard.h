#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <usb/hid/usb_hid.h>

#ifdef __cplusplus
extern "C" {
#endif

// Number of keys
#define KEYBOARD_KEYS		5

extern const uint32_t keyboard_masks[KEYBOARD_KEYS];

void keyboard_init(usb_hid_t *hid_keyboard);
uint32_t keyboard_status();

#ifdef __cplusplus
}
#endif

#endif // KEYBOARD_H
