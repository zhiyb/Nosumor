#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <debug.h>
#include <escape.h>
#include <vendor_defs.h>
#include <system/flash.h>
#include <peripheral/keyboard.h>
#include <peripheral/led.h>
#include <usb/hid/usb_hid.h>
#include "vendor.h"

#define UID	((uint32_t *)UID_BASE)

typedef struct PACKED {
	union PACKED {
		struct PACKED {
			uint16_t hw_ver;
		};
		uint32_t otp[16][8];
	};
	uint8_t lock[16];
} otp_t;

#define OTP	((otp_t *)FLASH_OTP_BASE)

static vendor_report_t report SECTION(.dtcm);

static void ping(usb_hid_if_t *hid)
{
	report.type = Ping | Reply;
	pong_t *p = (pong_t *)report.payload;
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
	report.size = VENDOR_REPORT_BASE_SIZE + sizeof(pong_t);
	usb_hid_vendor_send(hid, &report);
}

static void flash_check(usb_hid_if_t *hid)
{
	report.type = FlashCheck | Reply;
	report.size = VENDOR_REPORT_BASE_SIZE + 1u;
	int valid = flash_hex_check();
	report.payload[0] = valid;
	usb_hid_vendor_send(hid, &report);
	if (valid)
		puts(ESC_GOOD "Valid HEX content received");
	else
		puts(ESC_ERROR "Invalid HEX content received");
}

// OUT; Format: Num(8), Info[N](16)
static void vendor_led_info(usb_hid_if_t *hid)
{
	report.type = LEDInfo | Reply;
	report.size = VENDOR_REPORT_BASE_SIZE + 1u;
	const void *p = led_info(&report.payload[0]);
	memcpy(&report.payload[1], p, report.payload[0] * 2u);
	report.size += report.payload[0] * 2u;
	usb_hid_vendor_send(hid, &report);
}

// IN; Format: ID(8), R(16), G(16), B(16)
static void vendor_led_config(usb_hid_if_t *hid, uint8_t size, void *payload)
{
	uint8_t id;
	uint16_t clr[3];
	memcpy(&id, payload++, 1);
	if (!(id & 0x80)) {
		report.type = LEDConfig | Reply;
		report.size = VENDOR_REPORT_BASE_SIZE + 7u;
		report.payload[0] = id;
		led_get(id, 3u, clr);
		memcpy(&report.payload[1], clr, sizeof(clr));
		usb_hid_vendor_send(hid, &report);
	} else {
		if (size != 7) {
			dbgbkpt();
			return;
		}
		memcpy(clr, payload, sizeof(clr));
		led_set(id & 0x7f, 3u, clr);
	}
}

void vendor_process(usb_hid_if_t *hid, vendor_report_t *rp)
{
	if (!rp->size)
		return;
#if 0
	dbgprintf("\n" ESC_YELLOW "Vendor OUT report size %u, type %02x, content %02x %02x %02x",
		  rp->size, rp->type, rp->payload[0], rp->payload[1], rp->payload[2]);
#endif
	if (rp->size < VENDOR_REPORT_BASE_SIZE) {
		dbgbkpt();
		return;
	}
	uint8_t size = rp->size - VENDOR_REPORT_BASE_SIZE;
	switch (rp->type) {
	case Ping:
		ping(hid);
		break;
	case FlashReset:
		flash_hex_free();
		break;
	case FlashData:
		flash_hex_data(size, rp->payload);
		break;
	case FlashCheck:
		flash_check(hid);
		break;
	case FlashStart:
		flash_hex_program();
		break;
	case KeycodeUpdate:
		if (size != 2)
			break;
		keyboard_keycode_set(rp->payload[0], rp->payload[1]);
		break;
	case LEDInfo:
		vendor_led_info(hid);
		break;
	case LEDConfig:
		vendor_led_config(hid, size, rp->payload);
		break;
	}
}

static char toHex(char c)
{
	return (c < 10 ? '0' : 'a' - 10) + c;
}

void vendor_uid_str(char *s)
{
	// Construct serial number string from device UID
	char *c = s;
	uint8_t *p = (void *)UID + 11u;
	for (uint32_t i = 12; i != 0; i--) {
		*c++ = toHex(*p >> 4u);
		*c++ = toHex(*p-- & 0x0f);
	}
	*c = 0;
}
