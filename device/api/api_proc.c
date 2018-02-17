#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <debug.h>
#include <escape.h>
#include <api_defs.h>
#include <api_ping.h>
#include <api_keycode.h>
#include <api_led.h>
#include <usb/hid/usb_hid.h>
#include <usb/hid/vendor/usb_hid_vendor.h>
#include "api_proc.h"

#define UID	((uint32_t *)UID_BASE)

struct api_channel_t {
	struct api_channel_t *next;
	const struct api_reg_t *reg;
} *channels = 0;

static api_report_t report SECTION(.dtcm);

/* Info channel */

static void api_info_handler(void *hid, uint8_t channel,
			     void *data, uint8_t size, void *payload);
static const struct api_reg_t api_info = {
	api_info_handler, 0x0001, "Info"
};

static void api_info_handler(void *hid, uint8_t channel,
			     void *data, uint8_t size, void *payload)
{
	if (size != 1) {
		printf(ESC_ERROR "[INFO] Invalid size: %u\n", size);
		return;
	}

	api_info_t *p = data, *rp = (void *)payload;
	uint8_t ch = p->channel;
	const struct api_channel_t *cp;
	for (cp = channels; cp && ch; ch--)
		cp = cp->next;
	if (!cp) {
		printf(ESC_ERROR "[INFO] Invalid channel: %u\n",
		       p->channel);
		return;
	}

	// Construct reply packet
	size = strlen(cp->reg->name);
	rp->version = cp->reg->version;
	memcpy(rp->name, cp->reg->name, size);
	size += sizeof(struct api_info_t);

	// Report total channel number
	if (p->channel == 0) {
		uint8_t ch = 0;
		for (const struct api_channel_t *cp = channels; cp; ch++)
			cp = cp->next;
		rp->channel = ch;
	}

	api_send(hid, channel, size);
}

/* Common functions */

void api_register(const struct api_reg_t *reg)
{
	struct api_channel_t *channel = malloc(sizeof(struct api_channel_t));
	memcpy(channel, &(const struct api_channel_t){
		       .next = 0,
		       .reg = reg,
	       }, sizeof(struct api_channel_t));
	struct api_channel_t **cp = &channels;
	while (*cp)
		cp = &(*cp)->next;
	*cp = channel;
}

void api_send(void *hid, uint8_t channel, uint8_t size)
{
	report.size = API_BASE_SIZE + size;
	report.cksum = 0;
	report.channel = channel;

	// Checksum
	uint8_t c = 0, *p = report.raw;
	for (uint8_t i = report.size; i--;)
		c += *p++;
	report.cksum = -c;

	dbgprintf(ESC_DEBUG "[API] Report " ESC_WRITE "IN"
		  ESC_DEBUG ": size %u, channel %02x\n",
		  report.size, report.channel);

	usb_hid_vendor_send(hid, &report);
}

#if 0
static void flash_check(usb_hid_if_t *hid)
{
	report.type = FlashCheck | Reply;
	report.size = VENDOR_BASE_SIZE + 1u;
	int valid = flash_hex_check();
	report.payload[0] = valid;
	usb_hid_vendor_send(hid, &report);
	if (valid)
		puts(ESC_GOOD "Valid HEX content received");
	else
		puts(ESC_ERROR "Invalid HEX content received");
}
#endif

#ifndef BOOTLOADER
#if 0
static void vendor_i2c(usb_hid_if_t *hid, uint8_t size, void *payload)
{
	if (size-- == 0)
		return;

	uint8_t *p = payload++;
	uint8_t addr = *p++;

	report.type = I2CData | Reply;
	report.size = VENDOR_BASE_SIZE + 2u;
	report.payload[0] = addr;
	if (addr & 1u)
		report.payload[1] = i2c_read_reg(i2c, addr >> 1u, *p);
	else {
		addr >>= 1u;
		uint8_t ack = 0x81;
		uint8_t *p = payload;
		if (size == 0)
			ack &= i2c_check(i2c, addr);
		// Align to 2-byte register-value pairs
		for (size &= ~0x01; size; size -= 2u) {
			uint8_t reg = *p++;
			ack &= i2c_write_reg(i2c, addr, reg, *p++);
		}
		report.payload[1] = ack;
	}
	usb_hid_vendor_send(hid, &report);
}
#endif
#endif

void api_init(void *hid)
{
	report.id = usb_hid_vendor_id(hid);
	// Register API channels
	api_register(&api_info);
	api_register(&api_ping);
	api_register(&api_keycode);
#ifdef BOOTLOADER
	api_register(&api_led);
#endif
}

void api_process(void *hid, api_report_t *rp)
{
	if (!rp->size)
		return;

	dbgprintf(ESC_DEBUG "[API] Report " ESC_READ "OUT"
		  ESC_DEBUG ": size %u, channel %02x\n",
		  rp->size, rp->channel);

	// Check payload size
	if (rp->size < API_BASE_SIZE || rp->size > API_REPORT_SIZE) {
		printf(ESC_ERROR "[API] Invalid report size: %u\n", rp->size);
		dbgbkpt();
		return;
	}

	// Checksum
	uint8_t c = 0, *p = rp->raw;
	for (uint8_t i = rp->size; i--;)
		c += *p++;
	if (c != 0) {
		printf(ESC_ERROR "[API] Checksum failed\n");
		dbgbkpt();
		return;
	}

	uint8_t ch = rp->channel;
	const struct api_channel_t *cp = channels;
	while (cp && ch--)
		cp = cp->next;
	if (!cp || ++ch) {
		printf(ESC_ERROR "[API] Invalid channel: %u\n", ch);
		dbgbkpt();
		return;
	}

	uint8_t size = rp->size - API_BASE_SIZE;
	cp->reg->handler(hid, rp->channel, rp->payload, size, report.payload);

#if 0
	switch (rp->type) {
#ifndef BOOTLOADER
	case I2CData:
		vendor_i2c(hid, size, rp->payload);
		break;
#endif
	}
#endif
}

static char toHex(char c)
{
	return (c < 10 ? '0' : 'a' - 10) + c;
}

void uid_str(char *s)
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
