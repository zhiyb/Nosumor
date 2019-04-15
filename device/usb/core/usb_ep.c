#include <device.h>
#include <debug.h>
#include <escape.h>
#include <list.h>
#include "usb_ep.h"

#if 0
#include <system/systick.h>
#include "usb_ep.h"
#include "usb_ep0.h"
#include "usb_macros.h"
#include "usb_structs.h"
#endif

LIST(usb_ep_text, const usb_ep_t);

// Special link time arrays
// Note: GCC optimisation thinks pointer values from different sections must be different
//       So don't do equivalence check with these pointers
#define USB_EP_RESERVE_DATA(ep, dir) \
	static usb_ep_data_t _reserve_usb_ep_ ## dir ## _ ## ep[] \
		SECTION(".usb_ep_data." #dir "." #ep ".0") USED = {}

extern usb_ep_data_t _start_usb_ep_in_data[];
USB_EP_RESERVE_DATA(0x00, IN);
USB_EP_RESERVE_DATA(0x01, IN);
USB_EP_RESERVE_DATA(0x02, IN);
USB_EP_RESERVE_DATA(0x03, IN);
USB_EP_RESERVE_DATA(0x04, IN);
USB_EP_RESERVE_DATA(0x05, IN);
USB_EP_RESERVE_DATA(0x06, IN);
USB_EP_RESERVE_DATA(0x07, IN);
extern usb_ep_data_t _stop_usb_ep_in_data[];

extern usb_ep_data_t _start_usb_ep_out_data[];
USB_EP_RESERVE_DATA(0x00, OUT);
USB_EP_RESERVE_DATA(0x01, OUT);
USB_EP_RESERVE_DATA(0x02, OUT);
USB_EP_RESERVE_DATA(0x03, OUT);
USB_EP_RESERVE_DATA(0x04, OUT);
USB_EP_RESERVE_DATA(0x05, OUT);
USB_EP_RESERVE_DATA(0x06, OUT);
USB_EP_RESERVE_DATA(0x07, OUT);
extern usb_ep_data_t _stop_usb_ep_out_data[];

static usb_ep_data_t * const _reserve_usb_ep_in[] = {
	_reserve_usb_ep_IN_0x00, _reserve_usb_ep_IN_0x01, _reserve_usb_ep_IN_0x02, _reserve_usb_ep_IN_0x03,
	_reserve_usb_ep_IN_0x04, _reserve_usb_ep_IN_0x05, _reserve_usb_ep_IN_0x06, _reserve_usb_ep_IN_0x07,
};

static usb_ep_data_t * const _reserve_usb_ep_out[] = {
	_reserve_usb_ep_OUT_0x00, _reserve_usb_ep_OUT_0x01, _reserve_usb_ep_OUT_0x02, _reserve_usb_ep_OUT_0x03,
	_reserve_usb_ep_OUT_0x04, _reserve_usb_ep_OUT_0x05, _reserve_usb_ep_OUT_0x06, _reserve_usb_ep_OUT_0x07,
};

uint32_t usb_ep_reserve_segment(usb_ep_data_t *start, usb_ep_data_t *stop, uint32_t n_ep, const char *dir)
{
	// Available endpoints
	uint32_t eps[n_ep];
	for (uint32_t i = 0; i != n_ep; i++)
		eps[i] = 0;

	// Try to satisify requests first
	uint32_t ep = -1;
	for (usb_ep_data_t *p = start; p != stop; p++) {
		p->ep = EP_ANY;
		// Skip disabled and don't care endpoints
		if (!p->resv || p->ep_req == EP_ANY)
			continue;
		// Reserve endpoints that has a different endpoint number
		if (p->ep_req != ep && !eps[p->ep_req]) {
			ep = p->ep = p->ep_req;
			eps[ep] = 1;
			dbgprintf(ESC_DEBUG "[USB.Core] Reserved %s endpoint %lu to %p\n", dir, ep, p);
		}
	}

	// Reserve remaining endpoints
	for (usb_ep_data_t *p = start; p != stop; p++) {
		// Skip disabled and reserved endpoints
		if (!p->resv || p->ep != EP_ANY)
			continue;
		// Find the first available endpoint
		for (ep = 0; ep != n_ep && eps[ep]; ep++);
		if (ep != n_ep) {
			p->ep = ep;
			eps[ep] = 1;
			dbgprintf(ESC_DEBUG "[USB.Core] Reserved %s endpoint %lu to %p\n", dir, ep, p);
		} else {
			printf(ESC_ERROR "[USB.Core] No %s endpoint available for %p\n", dir, p);
			dbgbkpt();
		}
	}

	// Number of endpoints reserved
	ep = 0;
	for (uint32_t i = 0; i != n_ep; i++)
		if (eps[i])
			ep++;

	// Check for endpoint 0 reservation
	if (!eps[0]) {
		printf(ESC_ERROR "[USB.Core] %s endpoint 0 not reserved\n", dir);
		panic();
	}
	return ep;
}

void usb_ep_reserve()
{
	usb_ep_reserve_segment(_start_usb_ep_in_data, _stop_usb_ep_in_data, ASIZE(_reserve_usb_ep_in), "IN");
	usb_ep_reserve_segment(_start_usb_ep_out_data, _stop_usb_ep_out_data, ASIZE(_reserve_usb_ep_out), "OUT");
}

#if 0
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

void usb_ep_in_descriptor(USB_OTG_GlobalTypeDef *usb, int ep, desc_t desc, uint32_t zpkt)
{
	if (desc.size) {
		if (ep == 0) {
			uint32_t max = usb_ep0_max_size(usb);
			while (desc.size) {
				// Endpoint 0 can only transfer 127 bytes maximum
				uint32_t s = desc.size > 127u ? max : desc.size;
				usb_ep_in_transfer(usb, ep, desc.p, s);
				desc.p += s;
				desc.size -= s;
			}
		} else
			usb_ep_in_transfer(usb, ep, desc.p, desc.size);
		// 0-byte short packets
		if (zpkt)
			usb_ep_in_transfer(usb, ep, 0, 0);
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
	uint32_t tick = systick_cnt();
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
		if (systick_cnt() - tick >= 1024)
			dbgbkpt();
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
#endif
