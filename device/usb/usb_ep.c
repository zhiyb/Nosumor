#include <debug.h>
#include "usb_ep.h"
#include "usb_ep0.h"
#include "usb_macros.h"
#include "usb_structs.h"

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

uint32_t usb_ep_in_transfer(USB_OTG_GlobalTypeDef *usb, int n, const void *p, uint32_t size)
{
	// Wait for endpoint available
	if (!usb_ep_in_wait(usb, n))
		return 0;
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb, n);
	uint32_t pcnt = 1;
	if (size != 0) {
		uint32_t max = usb_ep_in_max_size(usb, n);
		pcnt = (size + max - 1) / max;
	}
	ep->DIEPTSIZ = (pcnt << USB_OTG_DIEPTSIZ_PKTCNT_Pos) | size;
	ep->DIEPDMA = (uint32_t)p;
	// Enable endpoint
	DIEPCTL_SET(ep->DIEPCTL, USB_OTG_DIEPCTL_EPENA_Msk | USB_OTG_DIEPCTL_CNAK_Msk);
	return size;
}

void usb_ep_in_descriptor(USB_OTG_GlobalTypeDef *usb, int ep, desc_t desc)
{
	if (desc.size) {
		if (ep == 0) {
			uint32_t max = usb_ep0_max_size(usb);
			while (desc.size) {
				// Endpoint 0 can only transfer 127 bytes maximum
				uint32_t s = desc.size > 127u ? max : desc.size;
				usb_ep_in_transfer(usb, ep, desc.p, s);
				// 0-byte short packet
				if (desc.size == max)
					usb_ep_in_transfer(usb, ep, 0, 0);
				desc.p += s;
				desc.size -= s;
			}
		} else
			usb_ep_in_transfer(usb, ep, desc.p, desc.size);
	} else
		usb_ep_in_stall(usb, ep);
}

void usb_ep_in_stall(USB_OTG_GlobalTypeDef *usb, int ep)
{
	DIEPCTL_SET(EP_IN(usb, ep)->DIEPCTL, USB_OTG_DIEPCTL_STALL_Msk);
}

uint32_t usb_ep_in_active(USB_OTG_GlobalTypeDef *usb, int ep)
{
	return (EP_IN(usb, ep)->DIEPCTL &
		(USB_OTG_DIEPCTL_EPENA_Msk | USB_OTG_DIEPCTL_NAKSTS_Msk)) ==
			USB_OTG_DIEPCTL_EPENA_Msk;
}

int usb_ep_in_wait(USB_OTG_GlobalTypeDef *usb, int n)
{
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb, n);
	// Wait for endpoint become available
	uint32_t ctl;
	while ((ctl = ep->DIEPCTL) & USB_OTG_DIEPCTL_EPENA_Msk) {
		if (!(ctl & USB_OTG_DIEPCTL_USBAEP_Msk)) {
			dbgbkpt();
			return 0;
		}
		if (ep->DIEPINT & USB_OTG_DIEPINT_TOC_Msk) {
			dbgbkpt();
			return 0;
		}
	}
	return 1;
}

uint32_t usb_ep_in_max_size(USB_OTG_GlobalTypeDef *usb, int ep)
{
	if (ep == 0)
		return usb_ep0_max_size(usb);
	return EP_IN(usb, ep)->DIEPCTL & USB_OTG_DIEPCTL_MPSIZ_Msk;
}

void usb_ep_out_transfer(USB_OTG_GlobalTypeDef *usb, int n, void *p,
			 uint8_t scnt, uint8_t pcnt, uint32_t size)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb, n);
	// Wait until endpoint become available
	while (ep->DOEPCTL & USB_OTG_DOEPCTL_EPENA_Msk);
	// Configure endpoint DMA
	ep->DOEPDMA = (uint32_t)p;
	// Reset packet counter
	ep->DOEPTSIZ = (scnt << USB_OTG_DOEPTSIZ_STUPCNT_Pos) |
			(pcnt << USB_OTG_DOEPTSIZ_PKTCNT_Pos) | size;
	// Enable endpoint OUT
	DOEPCTL_SET(ep->DOEPCTL, USB_OTG_DOEPCTL_EPENA_Msk | USB_OTG_DOEPCTL_CNAK_Msk);
}

void usb_ep_out_wait(USB_OTG_GlobalTypeDef *usb, int n)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb, n);
	// Wait for transfer complete
	while (!(ep->DOEPINT & USB_OTG_DOEPINT_XFRC_Msk));
	ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk;
}

void usb_ep_out_ack(USB_OTG_GlobalTypeDef *usb, int n)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb, n);
	DOEPCTL_SET(ep->DOEPCTL, USB_OTG_DOEPCTL_EPENA_Msk | USB_OTG_DOEPCTL_CNAK_Msk);
}
