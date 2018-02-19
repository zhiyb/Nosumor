#include <stm32f7xx.h>
#include <stdio.h>
#include <string.h>
#include <escape.h>
#include <peripheral/keyboard.h>
#include <api/api_proc.h>
#include "api_keycode.h"

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload);
const struct api_reg_t api_keycode = {
	handler, 0x0002, "Keycode"
};

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload)
{
	if (size > 2) {
		printf(ESC_ERROR "[KEYCODE] Invalid size: %u\n", size);
		return;
	}

	uint8_t *dp = data;
	uint8_t *pp = payload;
	switch (size) {
	case 0:
		// Report key count
		*pp = KEYBOARD_KEYS;
		api_send(hid, channel, 1u);
		break;
	case 1:	{
		// Report keycode and key name
		uint8_t btn = *dp;
		*pp++ = keyboard_keycode(btn);
		const char *cp = keyboard_name(btn);
		uint32_t len = strlen(cp);
		memcpy(pp, cp, len);
		api_send(hid, channel, len + 1u);
		break;
	}
	case 2: {
		// Set keycode
		api_keycode_t *p = (api_keycode_t *)dp;
		keyboard_keycode_set(p->btn, p->code);
		break;
	}
	}
}
