#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <debug.h>
#include <escape.h>
#include <peripheral/keyboard.h>
#include <usb/hid/usb_hid.h>
#include "vendor.h"
#include "vendor_defs.h"
#include "flash.h"

#define UID	((uint32_t *)0x1ff07a10)

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

static vendor_report_t report;

static void ping(usb_hid_if_t *hid)
{
	report.type = Pong;
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
	report.type = FlashStatus;
	report.size = VENDOR_REPORT_BASE_SIZE + 1u;
	int valid = flash_hex_check();
	report.payload[0] = valid;
	usb_hid_vendor_send(hid, &report);
	if (valid)
		puts(ESC_GREEN "Valid HEX content received");
	else
		puts(ESC_RED "Invalid HEX content received");
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
