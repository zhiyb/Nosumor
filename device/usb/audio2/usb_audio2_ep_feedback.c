#include <malloc.h>
#include "../usb.h"
#include "../usb_structs.h"
#include "../usb_macros.h"
#include "../usb_ep.h"
#include "../usb_ram.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_ep_data.h"

typedef struct {
	uint32_t cnt;
} epdata_t;

static void epin_init(usb_t *usb, uint32_t n)
{
	uint32_t size = 4u, addr = usb_ram_alloc(usb, &size);
	usb->base->DIEPTXF[n - 1] = DIEPTXF(addr, size);
	// Unmask interrupts
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
	ep->DIEPINT = 0u;
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	dev->DAINTMSK |= DAINTMSK_IN(n);
	// Configure endpoint
	ep->DIEPCTL = EP_TYP_ISOCHRONOUS | (n << USB_OTG_DIEPCTL_TXFNUM_Pos) |
			(size << USB_OTG_DIEPCTL_MPSIZ_Pos);
}

int usb_audio2_ep_feedback_register(usb_t *usb)
{
	epdata_t *epdata = calloc(1u, sizeof(epdata_t));
	if (!epdata)
		panic();

	const epin_t epin = {
		.data = epdata,
		.init = &epin_init,
		//.halt = &epin_halt,
	};
	int ep;
	usb_ep_register(usb, &epin, &ep, 0, 0);
	return ep;
}

void usb_audio2_ep_feedback_halt(usb_t *usb, int ep, int halt)
{
	;
}
