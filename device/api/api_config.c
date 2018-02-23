#include <stm32f7xx.h>
#include <stdio.h>
#include <string.h>
#include <escape.h>
#include <peripheral/keyboard.h>
#include <api/api_proc.h>
#include "api_config.h"
#include "api_config_priv.h"

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload);
const struct api_reg_t api_config = {
	handler, 0x0001, "Config"
};

struct api_config_data_t api_config_data = {
	.keyboard = 1, .mouse = 0, .joystick = 0,
	.microSD = 1, .flash = 0,
};

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload)
{
	static const struct config_t {
		void *p;
		uint8_t size;
		char name[16];
	} configs[] = {
		{&api_config_data.keyboard, 0u, "Keyboard"},
		{&api_config_data.mouse, 0u, "Mouse"},
		{&api_config_data.joystick, 0u, "Joystick"},
		{&api_config_data.microSD, 0u, "MicroSD"},
#ifdef DEBUG
		{&api_config_data.flash, 0u, "Flash"},
#endif
	};
	uint8_t *dp = data, *pp = payload;

	if (size == 0) {
		// Report total configurations
		*pp = ASIZE(configs);
		api_send(hid, channel, 1u);
		return;
	}

	uint8_t i = *dp++;
	if (i >= ASIZE(configs)) {
		printf(ESC_ERROR "[CONF] Invalid configuration: %u\n", size);
		return;
	}

	const struct config_t *c = &configs[i];
	uint8_t s = c->size ?: 1;

	if (--size == 0) {
		// Report configuration
		*pp++ = c->size;
		memcpy(pp, c->p, s);
		pp += s;
		uint8_t len = strlen(c->name);
		memcpy(pp, c->name, len);
		api_send(hid, channel, 1u + s + len);
	} else {
		if (size != s) {
			printf(ESC_ERROR "[CONF] Invalid size: %u\n", size);
			return;
		}
		// Set configuration
		memcpy(c->p, dp, s);
	}
}
