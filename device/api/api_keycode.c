#include <stm32f7xx.h>
#include <stdio.h>
#include <string.h>
#include <escape.h>
#include <peripheral/keyboard.h>
#include <logic/api_proc.h>
#include "api_keycode.h"

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload);
const struct api_reg_t api_keycode = {
	handler, 0x0001, "Keycode"
};

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload)
{
	if (size != 2) {
		printf(ESC_ERROR "[KEYCODE] Invalid size: %u\n", size);
		return;
	}

	api_keycode_t *p = (api_keycode_t *)data;
	keyboard_keycode_set(p->btn, p->code);
}
