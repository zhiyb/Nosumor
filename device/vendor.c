#include <malloc.h>
#include <stdio.h>
#include <debug.h>
#include <escape.h>
#include <usb/hid/usb_hid.h>
#include "vendor.h"
#include "vendor_defs.h"
#include "flash.h"

void vendor_flash_check(hid_t *hid)
{
	static vendor_report_t report = {
		.type = FlashStatus,
		.size = VENDOR_REPORT_BASE_SIZE + 1u,
	};
	int valid = flash_hex_check();
	report.payload[0] = valid;
	usb_hid_vendor_send(hid, &report);
	if (valid)
		puts(ESC_GREEN "Valid HEX content received");
	else
		puts(ESC_RED "Invalid HEX content received");
}

void vendor_process(hid_t *hid, vendor_report_t *rp)
{
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
	case FlashReset:
		flash_hex_free();
		break;
	case FlashData:
		flash_hex_data(size, rp->payload);
		break;
	case FlashCheck:
		vendor_flash_check(hid);
		break;
	case FlashStart:
		flash_hex_program();
		break;
	}
}
