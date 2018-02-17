#include <stm32f7xx.h>
#include <stdio.h>
#include <string.h>
#include <escape.h>
#include <peripheral/led.h>
#include <api/api_proc.h>
#include "api_led.h"

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload);
const struct api_reg_t api_led = {
	handler, 0x0001, "LED"
};

// OUT; Format: Num(8), Info[N](16)
static void info(void *hid, uint8_t channel,
		 uint8_t *dp, uint8_t size, uint8_t *pp)
{
	if (size != 0) {
		printf(ESC_ERROR "[LED] Invalid size: %u\n", size);
		return;
	}

	uint8_t n = 0;
	const void *p = led_info(&n);
	*pp++ = n;
	n *= 2u;
	memcpy(pp, p, n);
	api_send(hid, channel, 1u + n);
}

// IN; Format: ID(8), R(16), G(16), B(16)
static void config(void *hid, uint8_t channel,
		   uint8_t *dp, uint8_t size, uint8_t *pp)
{
	uint8_t id = *dp++;
	uint16_t clr[3];
	if (!(id & 0x80)) {
		if (size != 1) {
			printf(ESC_ERROR "[LED] Invalid size: %u\n", size);
			return;
		}
		*pp++ = id;
		led_get(id, 3u, clr);
		memcpy(pp, clr, sizeof(clr));
		api_send(hid, channel, 1u + sizeof(clr));
	} else {
		if (size != 7) {
			printf(ESC_ERROR "[LED] Invalid size: %u\n", size);
			return;
		}
		memcpy(clr, dp, sizeof(clr));
		led_set(id & 0x7f, 3u, clr);
	}
}

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload)
{
	uint8_t *p = data;
	size--;
	switch (*p++) {
	case LEDInfo:
		info(hid, channel, p, size, payload);
		break;
	case LEDConfig:
		config(hid, channel, p, size, payload);
		break;
	}
}
