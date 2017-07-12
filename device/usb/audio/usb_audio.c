#include <malloc.h>
#include "usb_audio.h"
#include "usb_audio_desc.h"
#include "../usb.h"
#include "../usb_debug.h"
#include "../usb_desc.h"
#include "../usb_ram.h"
#include "../usb_ep.h"

typedef struct data_t
{
	int ep_in, ep_out;
} data_t;

static void usbif_config(usb_t *usb, void *data)
{
	data_t *p = (data_t *)data;
	// Register endpoints
	const epin_t epin = {
		.data = data,
		//.init = &epin_init,
		//.halt = &epin_halt,
		//.xfr_cplt = &epin_xfr_cplt,
	};
	usb_ep_register(usb, &epin, &p->ep_in, 0, 0);

	// Audio control interface
	uint32_t s = usb_desc_add_string(usb, 0, LANG_EN_US, "USB Audio");
	usb_desc_add_interface(usb, 0u, 0u, AUDIO, AUDIOCONTROL, 0u, s);
	// Alternate setting 0, zero-bandwidth
	usb_desc_add_interface(usb, 0u, 0u, AUDIO, AUDIOSTREAMING, 0u, 0u);
	//usb_desc_add_interface(usb, 0u, 1u, AUDIO, AUDIOSTREAMING,
	//		       PR_PROTOCOL_UNDEFINED, s);
	//usb_desc_add(usb, &desc_hid, desc_hid.bLength);
	//usb_desc_add_endpoint(usb, EP_DIR_IN | p->ep_in,
	//		      EP_INTERRUPT, KEYBOARD_REPORT_SIZE, 1u);
}

void usb_audio_init(usb_t *usb)
{
	usb_if_t usbif = {
		.data = calloc(1u, sizeof(data_t)),
		.config = &usbif_config,
		//.enable = &usbif_enable,
		//.disable = &usbif_disable,
		//.setup_std = &usbif_setup_std,
		//.setup_class = &usbif_setup_class,
	};
	//_usb.usb = 0;
	usb_interface_alloc(usb, &usbif);
}
