#include <malloc.h>
#include "../usb.h"
#include "../usb_structs.h"
#include "../usb_macros.h"
#include "../usb_ep.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_ep_data.h"

typedef struct {
	void *data[2];
	int swap;
} epdata_t;

static void epout_recv(usb_t *usb, uint32_t n)
{
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	epdata_t *epdata = usb->epout[n].data;
	// Configure endpoint DMA
	ep->DOEPDMA = (uint32_t)epdata->data[epdata->swap];
	epdata->swap = !epdata->swap;
	// Reset packet counter
	ep->DOEPTSIZ = (1u << USB_OTG_DOEPTSIZ_PKTCNT_Pos) | DATA_MAX_SIZE;
	// Check frame parity
	uint32_t fn = FIELD(dev->DSTS, USB_OTG_DSTS_FNSOF);
	fn = (fn & 1) ? USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Msk : USB_OTG_DOEPCTL_SODDFRM_Msk;
	// Enable endpoint
	DOEPCTL_SET(ep->DOEPCTL, fn | USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK);
}

static void epout_init(usb_t *usb, uint32_t n)
{
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	// Set endpoint type
	ep->DOEPCTL = USB_OTG_DOEPCTL_USBAEP_Msk | USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Msk |
			EP_TYP_ISOCHRONOUS | DATA_MAX_SIZE;
	// Clear interrupts
	ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk;
	// Unmask interrupts
	dev->DAINTMSK |= DAINTMSK_OUT(n);
}

static void epout_halt(usb_t *usb, uint32_t n, int halt)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	uint32_t ctl = ep->DOEPCTL;
	audio_out_enable(!halt);
	if (!(ctl & USB_OTG_DOEPCTL_EPENA_Msk) != !halt)
		return;
	if (halt) {
		DOEPCTL_SET(ep->DOEPCTL, USB_OTG_DOEPCTL_EPDIS);
		//while (ep->DOEPCTL & USB_OTG_DOEPCTL_EPENA_Msk);
	} else {
		usb->epout[n].isoc_check = 1;
		epout_recv(usb, n);
	}
}

static void epout_xfr_cplt(usb_t *usb, uint32_t n)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	epdata_t *epdata = usb->epout[n].data;
	uint32_t siz = ep->DOEPTSIZ;
	// No more isochronous incomplete checks needed
	usb->epout[n].isoc_check = 0;
	epout_recv(usb, n);
	uint32_t pktcnt = 1u - FIELD(siz, USB_OTG_DOEPTSIZ_PKTCNT);
	uint32_t size = DATA_MAX_SIZE - FIELD(siz, USB_OTG_DOEPTSIZ_XFRSIZ);
	if (pktcnt)
		audio_play(epdata->data[epdata->swap], size);
}

int usb_audio2_ep_data_register(usb_t *usb)
{
	epdata_t *epdata = calloc(1u, sizeof(epdata_t));
	if (!epdata)
		panic();
	epdata->data[0] = malloc(DATA_MAX_SIZE);
	epdata->data[1] = malloc(DATA_MAX_SIZE);
	if (!epdata->data[0] || !epdata->data[1])
		panic();

	const epout_t epout = {
		.data = epdata,
		.init = &epout_init,
		.halt = &epout_halt,
		.xfr_cplt = &epout_xfr_cplt,
	};
	int ep;
	usb_ep_register(usb, 0, 0, &epout, &ep);
	return ep;
}

void usb_audio2_ep_data_halt(usb_t *usb, int ep, int halt)
{
	epout_halt(usb, ep, halt);
}
