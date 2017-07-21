#include <malloc.h>
#include "usb_audio.h"
#include "usb_audio_desc.h"
#include "../usb.h"
#include "../usb_debug.h"
#include "../usb_desc.h"
#include "../usb_setup.h"
#include "../usb_ram.h"
#include "../usb_ep.h"
#include "../../peripheral/audio.h"

#define EPOUT_MAX_SIZE	200u

typedef struct data_t
{
	int ep_out;
} data_t;

static void epout_init(usb_t *usb, uint32_t n)
{
	void *data = usb->epout[n].data;
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	// Configure endpoint DMA
	ep->DOEPDMA = (uint32_t)data;
	// Reset packet counter
	ep->DOEPTSIZ = (1u << USB_OTG_DOEPTSIZ_PKTCNT_Pos) | EPOUT_MAX_SIZE;
	// Set endpoint type
	ep->DOEPCTL = USB_OTG_DOEPCTL_USBAEP_Msk | EP_IN_TYP_ISOCHRONOUS | EPOUT_MAX_SIZE;
	// Clear interrupts
	ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk;
	// Unmask interrupts
	USB_OTG_DeviceTypeDef *dev = DEVICE(usb->base);
	dev->DAINTMSK |= DAINTMSK_OUT(n);
}

static void epout_halt(usb_t *usb, uint32_t n, int halt)
{
	USB_OTG_DeviceTypeDef *dev = DEVICE(usb->base);
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	uint32_t ctl = ep->DOEPCTL;
	if (!(ctl & USB_OTG_DOEPCTL_EPENA_Msk) != !halt)
		return;
	if (halt) {
		dbgprintf(ESC_RED "USB Audio out disabled\n");
		DOEPCTL_SET(ep->DOEPCTL, USB_OTG_DOEPCTL_EPDIS_Msk);
		//while (ep->DOEPCTL & USB_OTG_DOEPCTL_EPENA_Msk);
		return;
	}
	dbgprintf(ESC_RED "USB Audio out enabled\n");
	// Enable endpoint
	uint32_t fn = FIELD(dev->DSTS, USB_OTG_DSTS_FNSOF);
	fn = (fn & 1) ? 0 : USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Msk;
	DOEPCTL_SET(ep->DOEPCTL, USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK | fn);
}

static void epout_xfr_cplt(usb_t *usb, uint32_t n)
{
	dbgbkpt();
}

static void usbif_ac_config(usb_t *usb, void *data)
{
	if (!desc_ac.wTotalLength) {
		desc_ac.wTotalLength = desc_ac.bLength;
		const desc_itd_t *pit = desc_itd;
		for (uint32_t i = 0; i != ASIZE(desc_itd); i++)
			desc_ac.wTotalLength += (pit++)->bLength;
		const desc_otd_t *pot = desc_otd;
		for (uint32_t i = 0; i != ASIZE(desc_otd); i++)
			desc_ac.wTotalLength += (pot++)->bLength;
	}
	// Audio control interface
	uint32_t s = usb_desc_add_string(usb, 0, LANG_EN_US, "USB Audio");
	uint8_t iface = usb_desc_add_interface(usb, 0u, 0u, AUDIO, AUDIOCONTROL, 0u, s);
	desc_ac.baInterfaceNr[0] = iface + 1;
	usb_desc_add(usb, &desc_ac, desc_ac.bLength);
	const desc_itd_t *pit = desc_itd;
	for (uint32_t i = 0; i != ASIZE(desc_itd); i++, pit++)
		usb_desc_add(usb, pit, pit->bLength);
	const desc_otd_t *pot = desc_otd;
	for (uint32_t i = 0; i != ASIZE(desc_otd); i++, pot++)
		usb_desc_add(usb, pot, pot->bLength);
}

static void usbif_as_config(usb_t *usb, void *data)
{
	data_t *p = (data_t *)data;
	// Register endpoints
	const epout_t epout = {
		.data = malloc(EPOUT_MAX_SIZE),
		.init = &epout_init,
		.halt = &epout_halt,
		.xfr_cplt = &epout_xfr_cplt,
	};
	usb_ep_register(usb, 0, 0, &epout, &p->ep_out);

	// Audio streaming interface

	// Alternate setting 0, zero-bandwidth
	usb_desc_add_interface(usb, 0u, 0u, AUDIO, AUDIOSTREAMING, 0u, 0u);

	// Alternate setting 1, operational
	usb_desc_add_interface(usb, 1u, 1u, AUDIO, AUDIOSTREAMING, 0u, 0u);
	usb_desc_add(usb, &desc_as[0], desc_as[0].bLength);
	usb_desc_add(usb, &desc_pcm[0], desc_pcm[0].bLength);

	// Endpoint descriptor
	usb_desc_add_endpoint(usb, EP_DIR_OUT | p->ep_out,
			      EP_ISOCHRONOUS | EP_ISO_SYNC | EP_ISO_DATA,
			      EPOUT_MAX_SIZE, 1u);
	usb_desc_add(usb, &desc_ep[0], desc_ep[0].bLength);
}

static void usbif_as_enable(usb_t *usb, void *data)
{
	data_t *p = (data_t *)data;
	//epout_halt(usb, p->ep_out, 0);
}

static void usbif_as_disable(usb_t *usb, void *data)
{
	data_t *p = (data_t *)data;
	epout_halt(usb, p->ep_out, 1);
}

static void usbif_as_setup_std(usb_t *usb, void *data, uint32_t ep, setup_t pkt)
{
	switch (pkt.bmRequestType & SETUP_TYPE_DIR_H2D) {
	case SETUP_TYPE_DIR_H2D:
		switch (pkt.bRequest) {
		case SETUP_REQ_SET_INTERFACE:
			switch (pkt.wValue) {
			case 0:
				audio_out_enable(0);
				epout_halt(usb, ep, 1);
				usb_ep_in_transfer(usb->base, ep, 0, 0);
				break;
			case 1:
				audio_out_enable(1);
				epout_halt(usb, ep, 0);
				usb_ep_in_transfer(usb->base, ep, 0, 0);
				break;
			default:
				usb_ep_in_stall(usb->base, ep);
				dbgbkpt();
			}
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
			break;
		}
		break;
	default:
		dbgbkpt();
		break;
	}
}

void usb_audio_init(usb_t *usb)
{
	void *data = calloc(1u, sizeof(data_t));
	// Audio control interface
	const usb_if_t usbif_ac = {
		.data = data,
		.config = &usbif_ac_config,
	};
	usb_interface_alloc(usb, &usbif_ac);
	// Audio streaming interface
	const usb_if_t usbif_as = {
		.data = data,
		.config = &usbif_as_config,
		.enable = &usbif_as_enable,
		.disable = &usbif_as_disable,
		.setup_std = &usbif_as_setup_std,
	};
	usb_interface_alloc(usb, &usbif_as);
}
