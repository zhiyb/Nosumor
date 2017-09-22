#include <malloc.h>
#include <debug.h>
#include "../usb.h"
#include "../usb_structs.h"
#include "../usb_ram.h"
#include "usb_msc.h"
#include "usb_msc_defs.h"

typedef struct usb_msc_t {
} usb_msc_t;

static void usbif_config(usb_t *usb, void *pdata)
{
	usb_msc_t *data = (usb_msc_t *)pdata;

	uint32_t s = usb_desc_add_string(usb, 0, LANG_EN_US, "USB Mass Storage");
	usb_desc_add_interface(usb, 0u, 2u, MASS_STORAGE, RBC, BBB, s);

	// Register endpoints
}

usb_msc_t *usb_msc_init(usb_t *usb)
{
	usb_msc_t *data = calloc(1u, sizeof(usb_msc_t));
	if (!data)
		panic();
	// Audio control interface
	const usb_if_t usbif = {
		.data = data,
		.config = &usbif_config,
		//.setup_class = &usbif_setup_class,
	};
	usb_interface_register(usb, &usbif);
	return data;
}
