#include <string.h>
#include <usb/usb_hw.h>
#include <usb/usb_desc.h>
#include <usb/usb_macros.h>
#include "usb_hid.h"

#define HID_IN_MAX_SIZE		48u
#define HID_OUT_MAX_PKT		4u
#define HID_OUT_MAX_SIZE	48u

typedef struct PACKED desc_hid_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdHID;
	uint8_t bCountryCode;
	uint8_t bNumDescriptors;
	uint8_t bClassDescriptorType;
	uint16_t wDescriptorLength;
} desc_hid_t;

static const desc_hid_t desc_hid = {
	// 0x21: HID descriptor
	// 0x22: Report descriptor
	9u, 0x21u, 0x0111u, 0x00u, 1u, 0x22u, 0u,
};

typedef enum {BufFree = 0, BufActive} buf_state_t;

struct {
	struct {
		uint32_t num;
		struct buf_t {
			void *p;
			volatile uint32_t xfrsize;
			volatile buf_state_t state;
		} buf[2];
		volatile uint32_t buf_queue;
	} epin, epout;
} data;

static void usb_hid_enumeration(uint32_t spd)
{
	UNUSED(spd);
	data.epin.num = usb_hw_ep_alloc(UsbEpNormal, EP_DIR_IN, EP_INTERRUPT, HID_IN_MAX_SIZE);
	data.epout.num = usb_hw_ep_alloc(UsbEpNormal, EP_DIR_OUT, EP_INTERRUPT, HID_OUT_MAX_SIZE);
}

USB_ENUM_HANDLER(&usb_hid_enumeration);

static void usb_hid_ep_out();
static void usb_hid_ep_irq_xfrc(void *p, uint32_t size);

static void usb_hid_config(uint8_t config)
{
	UNUSED(config);

	// Initialise endpoints

	// Allocate DMA memories
	data.epin.buf[0].p = usb_hw_ram_alloc(HID_IN_MAX_SIZE);
	data.epin.buf[0].state = BufFree;
	data.epin.buf[1].p = usb_hw_ram_alloc(HID_IN_MAX_SIZE);
	data.epin.buf[1].state = BufFree;
	data.epin.buf_queue = 0;
	data.epout.buf[0].p = usb_hw_ram_alloc(HID_OUT_MAX_SIZE);
	data.epout.buf[0].state = BufFree;
	data.epout.buf[1].p = usb_hw_ram_alloc(HID_OUT_MAX_SIZE);
	data.epout.buf[1].state = BufFree;
	data.epout.buf_queue = 0;

	usb_hw_ep_out_irq(data.epin.num, 0, 0, &usb_hid_ep_irq_xfrc);
	usb_hid_ep_out();
	usb_hw_ep_in_nak(data.epout.num);
}

USB_CONFIG_HANDLER(&usb_hid_config);

static void usb_hid_ep_out()
{
	struct buf_t *buf = &data.epout.buf[data.epout.buf_queue];
#ifdef DEBUG
	if (buf->state != BufFree)
		USB_ERROR();
#endif
	usb_hw_ep_out(data.epout.num, buf->p, 0, 1, 0);
	buf->state = BufActive;
}

static void usb_hid_ep_irq_xfrc(void *p, uint32_t size)
{
	USB_TODO();
}

LIST(usb_hid_desc, usb_hid_desc_handler_t);

static void usb_hid_desc(usb_hid_desc_t *pdesc)
{
	LIST_ITERATE(usb_hid_desc, usb_hid_desc_handler_t, pfunc) (*pfunc)(pdesc);
}

static void usb_hid_desc_config(usb_desc_t *pdesc)
{
	usb_hid_desc_t drep;
	char buf[pdesc->size];
	drep.p = buf;
	drep.len = 0;
	drep.size = pdesc->size;
	usb_hid_desc(&drep);

	if (drep.len == 0)
		return;

	desc_hid_t desc;
	desc = desc_hid;
	desc.wDescriptorLength = drep.len;

	// bInterfaceClass	3: HID class
	// bInterfaceSubClass	1: Boot interface
	// bInterfaceProtocol	0: None, 1: Keyboard, 2: Mouse
	usb_desc_add_interface(pdesc, 0u, 2u, 3u, 1u, 0u, 0);
	usb_desc_add(pdesc, &desc, desc.bLength);
	usb_desc_add_endpoint(pdesc, EP_DIR_IN | data.epin.num, EP_INTERRUPT, HID_IN_MAX_SIZE, 1u);
	usb_desc_add_endpoint(pdesc, EP_DIR_OUT | data.epout.num, EP_INTERRUPT, HID_OUT_MAX_SIZE, 1u);
}

USB_DESC_CONFIG_HANDLER(usb_hid_desc_config);
