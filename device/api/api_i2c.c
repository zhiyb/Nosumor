#include <stm32f7xx.h>
#include <stdio.h>
#include <string.h>
#include <escape.h>
#include <peripheral/i2c.h>
#include <api/api_proc.h>
#include "api_led.h"

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload);
const struct api_reg_t api_i2c = {
	handler, 0x0001, "I2C"
};

extern void *i2c;

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload)
{
	if (size == 0) {
		printf(ESC_ERROR "[I2C] Invalid size: %u\n", size);
		return;
	}
	size--;

	uint8_t *p = data, *pp = payload;
	uint8_t addr = *pp++ = *p++;
	if (addr & 1u) {
		*pp = i2c_read_reg(i2c, addr >> 1u, *p);
	} else {
		addr >>= 1u;
		uint8_t ack = 0x81;
		if (size == 0)
			ack &= i2c_check(i2c, addr);
		// Align to 2-byte register-value pairs
		for (size &= ~0x01; size; size -= 2u) {
			uint8_t reg = *p++;
			ack &= i2c_write_reg(i2c, addr, reg, *p++);
		}
		*pp = ack;
	}
	api_send(hid, channel, 2u);
}
