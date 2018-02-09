#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <usb/hid/usb_hid.h>

#ifdef __cplusplus
extern "C" {
#endif

// Number of keys
#define KEYBOARD_KEYS	5

#if HWVER >= 0x0100
// KEY_12:	K1(PB4), K2(PB2)
// KEY_345:	K3(PC13), K4(PC14), K5(PC15)
#define KEYBOARD_MASK_1	(1ul << 4)
#else
// KEY_12:	K1(PA6), K2(PB2)
// KEY_345:	K3(PC13), K4(PC14), K5(PC15)
#define KEYBOARD_MASK_1	(1ul << 6)
#endif
#define KEYBOARD_MASK_2	(1ul << 2)
#define KEYBOARD_MASK_3	(1ul << 13)
#define KEYBOARD_MASK_4	(1ul << 14)
#define KEYBOARD_MASK_5	(1ul << 15)

extern const uint32_t keyboard_masks[KEYBOARD_KEYS];

void keyboard_init(usb_hid_if_t *hid_keyboard, usb_hid_if_t *hid_joystick);
uint32_t keyboard_status();
void keyboard_keycode_set(unsigned int btn, uint8_t code);

#ifdef __cplusplus
}
#endif

#endif // KEYBOARD_H
