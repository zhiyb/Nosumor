#include <stm32f7xx.h>
#include <stdio.h>
#include <string.h>
#include <escape.h>
#include <system/flash.h>
#include <logic/api_proc.h>
#include "api_ping.h"

#define UID	((uint32_t *)UID_BASE)
#define OTP	((otp_t *)FLASH_OTP_BASE)

typedef struct PACKED {
	union PACKED {
		struct PACKED {
			uint16_t hw_ver;
		};
		uint32_t otp[16][8];
	};
	uint8_t lock[16];
} otp_t;

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload);
const struct api_reg_t api_ping = {
	handler, 0x0001, "Ping"
};

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload)
{
	if (size != 0) {
		printf(ESC_ERROR "[PING] Invalid size: %u\n", size);
		return;
	}

	api_pong_t *p = (api_pong_t *)payload;
	p->sw_ver = SW_VERSION;
#ifdef DEBUG
	p->sw_ver |= 0x8000;
#endif
#ifdef BOOTLOADER
	p->sw_ver |= 0x4000;
#endif
	p->hw_ver = OTP->hw_ver;
	memcpy(p->uid, UID, 12u);
	p->fsize = FLASH_SIZE;

	api_send(hid, channel, sizeof(api_pong_t));
}
