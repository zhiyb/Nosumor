#include <string.h>
#include <usb/hid/usb_hid.h>
#include <usb/usb_macros.h>
#include <usb/usb_hw.h>
#include "usb_hid_keyboard.h"

#define REPORT_SIZE	9

typedef enum {UsbHidBufFree = 0, UsbHidBufValid, UsbHidBufActive} usb_hid_report_buf_state_t;

static struct {
	struct {
		volatile usb_hid_report_buf_state_t state;
		usb_hid_report_t report;
	} buf[2];
	volatile uint32_t bufidx;
} data;

// Allocate buffer after enumeration

static void usb_hid_keyboard_report_cb(usb_hid_report_t *prpt);

static void usb_hid_keyboard_enum(uint32_t spd)
{
	UNUSED(spd);

	data.buf[0].state = UsbHidBufFree;
	data.buf[0].report.len = REPORT_SIZE;
	data.buf[0].report.p = usb_hw_ram_alloc(REPORT_SIZE);
	data.buf[0].report.cb = &usb_hid_keyboard_report_cb;
	data.buf[1].state = UsbHidBufFree;
	data.buf[1].report.len = REPORT_SIZE;
	data.buf[1].report.p = usb_hw_ram_alloc(REPORT_SIZE);
	data.buf[1].report.cb = &usb_hid_keyboard_report_cb;
	data.bufidx = 0;
}

USB_ENUM_HANDLER(&usb_hid_keyboard_enum);

// Report descriptor

static const uint8_t desc_report[] = {
	// Keyboard
	0x05, 0x01,		// Usage page (Generic desktop)
	0x09, 0x06,		// Usage (Keyboard)
	0xa1, 0x01,		// Collection (Application)
	0x85, 0,		//   Report ID
#if 1	// Modifier keys
	0x75, 0x01,		//   Report size (1)
	0x95, 0x08,		//   Report count (8)
	0x05, 0x07,		//   Usage page (Keyboard)
	0x19, 0xe0,		//   Usage minimum (Left control)
	0x29, 0xe7,		//   Usage maximum (Right GUI)
	0x15, 0x00,		//   Logical minimum (0)
	0x25, 0x01,		//   Logical maximum (1)
	0x81, 0x02,		//   Input (Data, Var, Abs)
#endif
#if 1	// Reserved
	0x95, 0x01,		//   Report count (1)
	0x75, 0x08,		//   Report size (8)
	0x81, 0x01,		//   Input (Cnst)
#endif
#if 0	// LEDs
	0x95, 0x08,		//   Report count (8)
	0x75, 0x01,		//   Report size (1)
	0x05, 0x08,		//   Usage page (LEDs)
	0x19, 0x01,		//   Usage minimum (Num lock)
	0x29, 0x07,		//   Usage maximum (Shift)
	0x91, 0x02,		//   Output (Data, Var, Abs)
	0x95, 0x01,		//   Report count (1)
	0x75, 0x01,		//   Report size (1)
	0x91, 0x01,		//   Output (Cnst)
#endif
	// Keys
	0x95, 0x06,		//   Report count (6)
	0x75, 0x08,		//   Report size (8)
	0x05, 0x07,		//   Usage page (Keyboard)
	0x19, 0x00,		//   Usage minimum (No event)
	0x29, 0xe7,		//   Usage maximum (Right GUI)
	0x15, 0x00,		//   Logical minimum (0)
	0x26, 0xe7, 0x00,	//   Logical maximum (231)
	0x81, 0x00,		//   Input (Data, Ary)
	0xc0,			// End collection
};

static void usb_hid_keyboard_desc(usb_hid_desc_t *pdesc);

USB_HID_DESC_HANDLER_LIST();
USB_HID_DESC_HANDLER(usb_hid_keyboard_desc_item) = &usb_hid_keyboard_desc;

static void usb_hid_keyboard_desc(usb_hid_desc_t *pdesc)
{
#if DEBUG >= 5
	if (pdesc->size < pdesc->len + sizeof(desc_report))
		USB_ERROR();
#endif
	memcpy(pdesc->p, desc_report, sizeof(desc_report));
	// Update report ID
	uint8_t id = USB_HID_DESC_HANDLER_ID(usb_hid_keyboard_desc_item);
	*(uint8_t *)(pdesc->p + pdesc->len + 7) = id;
	pdesc->len += sizeof(desc_report);
}

// Send report

void usb_hid_keyboard_report_update(usb_hid_keyboard_report_t *p)
{
	// Switch to alternate buffer if current buffer is active
	__disable_irq();
	uint32_t idx = data.bufidx;
	if (data.buf[idx].state == UsbHidBufActive) {
		data.bufidx = idx = !idx;
#if DEBUG >= 5
		if (data.buf[idx].state == UsbHidBufActive)
			USB_ERROR();
#endif
	}
	data.buf[idx].state = UsbHidBufFree;
	__enable_irq();

	// Fill buffer
	p->id = USB_HID_DESC_HANDLER_ID(usb_hid_keyboard_desc_item);
	memcpy(data.buf[idx].report.p, p, REPORT_SIZE);

	// Start HID transfer if not currently active
	__disable_irq();
	data.buf[idx].state = UsbHidBufValid;
	usb_hid_report_buf_state_t state = data.buf[!idx].state;
	if (state != UsbHidBufActive)
		data.buf[idx].state = UsbHidBufActive;
	__enable_irq();
	if (state != UsbHidBufActive)
		usb_hid_report_enqueue(&data.buf[idx].report);
}

static void usb_hid_keyboard_report_cb(usb_hid_report_t *prpt)
{
	// Check if pending buffer valid
	__disable_irq();
	uint32_t idx = data.bufidx;
	data.buf[idx].state = UsbHidBufFree;
	data.bufidx = idx = !idx;
	usb_hid_report_buf_state_t state = data.buf[idx].state;
	if (state == UsbHidBufValid)
		data.buf[idx].state = UsbHidBufActive;
	__enable_irq();
	// If valid, start new HID transfer
	if (state == UsbHidBufValid)
		usb_hid_report_enqueue(&data.buf[idx].report);
}
