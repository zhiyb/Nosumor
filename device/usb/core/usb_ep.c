#include <device.h>
#include <debug.h>
#include <escape.h>
#include <list.h>
#include "usb_ep.h"
#include "usb_ep0.h"

#if 0
#include <system/systick.h>
#include "usb_ep.h"
#include "usb_ep0.h"
#include "usb_macros.h"
#include "usb_structs.h"
#endif

extern const usb_ep_t _start_usb_ep_in_text[];
extern const usb_ep_t _start_usb_ep_out_text[];

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

static uint32_t fifo_size = 0, fifo_top = 0;

static void usb_ep_fifo_reset(USB_OTG_GlobalTypeDef *base)
{
	// TODO support for multiple USB instances
	if (!fifo_size)
		fifo_size = base->GRXFSIZ;
	fifo_top = 0;
}

static uint32_t usb_ep_fifo_alloc(uint32_t size)
{
	if (fifo_top + size >= fifo_size) {
		printf(ESC_ERROR "[USB.Core] Insufficient space in USB FIFO RAM\n");
		return 0;
	}
	fifo_top += size;
	return fifo_top;
}

#ifdef DEBUG
static void usb_ep_fifo_report(USB_OTG_GlobalTypeDef *base)
{
	printf(ESC_DEBUG "[USB.Core] OUT EP FIFO: 0 + %lu\n", base->GRXFSIZ);
	printf(ESC_DEBUG "[USB.Core] IN EP 0 FIFO: %lu + %lu\n",
	       (base->DIEPTXF0_HNPTXFSIZ & USB_OTG_DIEPTXF_INEPTXSA_Msk) >> USB_OTG_DIEPTXF_INEPTXSA_Pos,
	       (base->DIEPTXF0_HNPTXFSIZ & USB_OTG_DIEPTXF_INEPTXFD_Msk) >> USB_OTG_DIEPTXF_INEPTXFD_Pos);
	for (uint32_t ep = 1; ep != ASIZE(_reserve_usb_ep_in); ep++) {
		printf(ESC_DEBUG "[USB.Core] IN EP %lu FIFO: %lu + %lu\n", ep,
		       (base->DIEPTXF[ep - 1] & USB_OTG_DIEPTXF_INEPTXSA_Msk) >> USB_OTG_DIEPTXF_INEPTXSA_Pos,
		       (base->DIEPTXF[ep - 1] & USB_OTG_DIEPTXF_INEPTXFD_Msk) >> USB_OTG_DIEPTXF_INEPTXFD_Pos);
	}
}
#endif

static uint32_t usb_ep_reserve(usb_ep_data_t *start, usb_ep_data_t *p, const uint32_t ep, const uint32_t dir)
{
	const usb_ep_t *ptext = dir == HASH("IN") ? _start_usb_ep_in_text : _start_usb_ep_out_text;
	p->ep = ep;
	dbgprintf(ESC_DEBUG "[USB.Core] Reserved %s endpoint %lu to %s\n",
		  dir == HASH("IN") ? "IN" : "OUT", ep, (ptext + (p - start))->name);
	return 1;
}

static void usb_ep_reserve_segment(USB_OTG_GlobalTypeDef *base,
				   usb_ep_data_t *start, usb_ep_data_t *stop, const uint32_t n_ep, const uint32_t dir)
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
		if (p->ep_req != ep && !eps[p->ep_req])
			if (usb_ep_reserve(start, p, p->ep_req, dir))
				eps[ep = p->ep] = 1;
	}

	// Reserve remaining endpoints
	for (usb_ep_data_t *p = start; p != stop; p++) {
		// Skip disabled and reserved endpoints
		if (!p->resv || p->ep != EP_ANY)
			continue;
		// Find the first available endpoint
		for (ep = 0; ep != n_ep && eps[ep]; ep++);
		if (ep != n_ep) {
			if (usb_ep_reserve(start, p, ep, dir))
				eps[ep] = 1;
		} else {
			const usb_ep_t *ptext = dir == HASH("IN") ? _start_usb_ep_in_text : _start_usb_ep_out_text;
			printf(ESC_ERROR "[USB.Core] No %s endpoint available for 0x%08lx\n",
			       dir == HASH("IN") ? "IN" : "OUT", (ptext + (p - start))->id);
			dbgbkpt();
		}
	}

#ifdef DEBUG
	// Check for endpoint 0 reservation
	if (!eps[0]) {
		printf(ESC_ERROR "[USB.Core] %s endpoint 0 not reserved\n",
		       dir == HASH("IN") ? "IN" : "OUT");
		panic();
	}
#endif

	// Reserve FIFO RAM
	if (dir == HASH("IN")) {
		for (ep = 0; ep != n_ep; ep++) {
			if (!eps[ep]) {
				// Disabled endpoint
				base->DIEPTXF[ep - 1] = DIEPTXF(0, 0);
				continue;
			}
			// Find enabled endpoint
			usb_ep_data_t *p = start;
			for (usb_ep_data_t *p = start; p != stop && (!p->resv || p->ep != ep); p++);
#ifdef DEBUG
			if (p == stop) {
				dbgprintf(ESC_ERROR "[USB.Core] IN endpoint %lu data not found\n", ep);
				panic();
			}
#endif
			// Reserve transmit FIFO RAM
			const usb_ep_t *ptext = _start_usb_ep_in_text + (p - start);
			uint32_t size = (ptext->pkt_size + 3) / 4 * 2;
			if (size < 16)
				size = 16;	// Minimum size is 16 words
			__IO uint32_t *pio = ep == 0 ? &base->DIEPTXF0_HNPTXFSIZ : &base->DIEPTXF[ep - 1];
			*pio = DIEPTXF(fifo_top, size);
			if (!usb_ep_fifo_alloc(size)) {
				printf(ESC_ERROR "[USB.Core] Unable to allocate transmit FIFO of size %lu "
						 "for 0x%08lx IN endpoint %lu\n", size, ptext->id, ep);
				// Unreserve FIFO and disable endpoint
				*pio = DIEPTXF(0, 0);
				p->ep = EP_ANY;
				eps[ep] = 0;
			} else {
#ifdef DEBUG
				dbgprintf(ESC_DEBUG "[USB.Core] Allocated transmit FIFO of size %lu (%lu%%) "
						    "for %s IN endpoint %lu\n",
					  size, fifo_top * 100 / fifo_size, ptext->name, ep);
#endif
			}
		}
	} else {
		// Number of endpoints reserved
		ep = 0;
		// Number of control endpoints
		uint32_t ctrl_ep = 0;
		// Find largest packet size
		uint32_t size = 0;
		for (uint32_t i = 0; i != n_ep; i++) {
			if (eps[i]) {
				const usb_ep_t *ptext = _start_usb_ep_in_text + i;
				uint32_t pkt_size = ptext->pkt_size;
				if (pkt_size >= size)
					size = pkt_size;
				ctrl_ep += ptext->type == EP_CONTROL;
				ep++;
			}
		}
		// Reserve receive FIFO RAM
		size  = (size * 2 + 3) / 4 + 1;	// Reserve 2 largest packets with 1 status info
		size += 5 * ctrl_ep;		// Reserve 2 SETUP packets for each control endpoint
		size += 8;				// Reserve 3 SETUP packets for hardware
		size += 2 * ep;			// Reserve xfer complete status for each OUT endpoint
		size += 1;				// Reserve global OUT NAK
		if (!usb_ep_fifo_alloc(size)) {
			printf(ESC_ERROR "[USB.Core] Unable to allocate receive FIFO of size %lu\n", size);
			panic();
		} else {
			dbgprintf(ESC_DEBUG "[USB.Core] Allocated receive FIFO of size %lu (%lu%%) "
					    "for all endpoints\n", size,
				  fifo_top * 100 / fifo_size);
			base->GRXFSIZ = size;
		}
	}
}

static void usb_ep_reserve_all(USB_OTG_GlobalTypeDef *base)
{
	usb_ep_fifo_reset(base);
	// Reserve OUT (receive FIFO) first
	usb_ep_reserve_segment(base, _start_usb_ep_out_data, _stop_usb_ep_out_data, ASIZE(_reserve_usb_ep_out), HASH("OUT"));
	usb_ep_reserve_segment(base, _start_usb_ep_in_data, _stop_usb_ep_in_data, ASIZE(_reserve_usb_ep_in), HASH("IN"));
#ifdef DEBUG
	// FIFO usage report
	usb_ep_fifo_report(base);
#endif
}

void usb_ep_active(USB_OTG_GlobalTypeDef *base)
{
	usb_ep_reserve_all(base);
	// Call endpoint activate functions
	for (uint32_t i = 0; i != (uint32_t)(_stop_usb_ep_out_data - _start_usb_ep_out_data); i++) {
		const usb_ep_data_t *p = _start_usb_ep_out_data + i;
		const usb_ep_t *ptext = _start_usb_ep_out_text + i;
		if (p->resv && p->ep != EP_ANY)
			FUNC(ptext->activate)();
	}
	for (uint32_t i = 0; i != (uint32_t)(_stop_usb_ep_in_data - _start_usb_ep_in_data); i++) {
		const usb_ep_data_t *p = _start_usb_ep_in_data + i;
		const usb_ep_t *ptext = _start_usb_ep_in_text + i;
		if (p->resv && p->ep != EP_ANY)
			FUNC(ptext->activate)();
	}
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
