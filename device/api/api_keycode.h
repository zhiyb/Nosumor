#ifndef API_KEYCODE_H
#define API_KEYCODE_H

#include "api_defs.h"

typedef struct PACKED api_keycode_t {
	uint8_t btn;
	uint8_t code;
} api_keycode_t;

extern const struct api_reg_t api_keycode;

#endif // API_KEYCODE_H
