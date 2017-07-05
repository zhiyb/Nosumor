#include <debug.h>
#include "../escape.h"
#include "usb_ep.h"
#include "usb_ep0.h"
#include "usb_setup.h"
#include "usb_macros.h"

void usb_ep_register(usb_t *usb, const epin_t *epin, int *in, const epout_t *epout, int *out)
{
	if (epin) {
		usb->epin[usb->nepin] = *epin;
		if (in)
			*in = usb->nepin;
		usb->nepin++;
	}
	if (epout) {
		usb->epout[usb->nepout] = *epout;
		if (out)
			*out = usb->nepout;
		usb->nepout++;
	}
}

void usb_ep_in_stall(USB_OTG_GlobalTypeDef *usb, int ep)
{
	DIEPCTL_SET(EP_IN(usb, ep)->DIEPCTL, USB_OTG_DIEPCTL_STALL_Msk);
}

void usb_ep_in_transfer(USB_OTG_GlobalTypeDef *usb, int n, const void *p, uint32_t size)
{
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb, n);
	// Wait for endpoint available
	while (ep->DIEPCTL & USB_OTG_DIEPCTL_EPENA_Msk);
	if (size == 0) {
		ep->DIEPTSIZ = (1ul << USB_OTG_DIEPTSIZ_PKTCNT_Pos) | 0;
		DIEPCTL_SET(ep->DIEPCTL, USB_OTG_DIEPCTL_EPENA_Msk | USB_OTG_DIEPCTL_CNAK_Msk);
		dbgprintf("<%dI>\n", n);
		return;
	}

	dbgprintf(ESC_GREY "<%dI%lu ", n, size);
	uint8_t *dp = (uint8_t *)p;
	uint32_t i = size;
	while (i--)
		dbgprintf("%02x", *dp++);

	uint32_t max = usb_ep_in_max_size(usb, n);
	uint32_t pcnt = (size + max - 1) / max;
	ep->DIEPTSIZ = (pcnt << USB_OTG_DIEPTSIZ_PKTCNT_Pos) | size;
	// Enable endpoint
	DIEPCTL_SET(ep->DIEPCTL, USB_OTG_DIEPCTL_EPENA_Msk | USB_OTG_DIEPCTL_CNAK_Msk);
	// Write FIFO
	const uint32_t *ptr = p;
	size = (size + 3u) / 4u;
	while (size--)
		FIFO(usb, n) = *ptr++;

	dbgprintf(">\n");
}

uint32_t usb_ep_in_max_size(USB_OTG_GlobalTypeDef *usb, int ep)
{
	if (ep == 0)
		return usb_ep0_max_size(usb);
	return EP_IN(usb, ep)->DIEPCTL & USB_OTG_DIEPCTL_MPSIZ_Msk;
}
