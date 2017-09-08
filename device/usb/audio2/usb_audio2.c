#include <malloc.h>
#include <string.h>
#include "../../debug.h"
#include "../usb.h"
#include "../usb_structs.h"
#include "../usb_setup.h"
#include "../usb_ram.h"
#include "../usb_ep.h"
#include "usb_audio2.h"
#include "usb_audio2_desc.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_entities.h"

static void epout_recv(usb_t *usb, uint32_t n)
{
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	epdata_t *epdata = usb->epout[n].data;
	// Configure endpoint DMA
	ep->DOEPDMA = (uint32_t)epdata->data[epdata->swap];
	epdata->swap = !epdata->swap;
	// Reset packet counter
	ep->DOEPTSIZ = (1u << USB_OTG_DOEPTSIZ_PKTCNT_Pos) | EPOUT_MAX_SIZE;
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
			EP_TYP_ISOCHRONOUS | EPOUT_MAX_SIZE;
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
		epout_recv(usb, n);
	}
}

static void epout_xfr_cplt(usb_t *usb, uint32_t n)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	epdata_t *epdata = usb->epout[n].data;
	uint32_t siz = ep->DOEPTSIZ;
	epout_recv(usb, n);
	uint32_t pktcnt = 1u - FIELD(siz, USB_OTG_DOEPTSIZ_PKTCNT);
	uint32_t size = EPOUT_MAX_SIZE - FIELD(siz, USB_OTG_DOEPTSIZ_PKTCNT);
	//dbgprintf(ESC_BLUE "<X|%lx|%lx>", pktcnt, size);
	if (pktcnt) {
		audio_play(epdata->data[epdata->swap], size);
	}
}

static void usbif_ac_config(usb_t *usb, void *pdata)
{
	data_t *data = (data_t *)pdata;
	if (!desc_ac.wTotalLength) {
		desc_ac.wTotalLength = desc_ac.bLength;
		// Clock sources
		const desc_cs_t *pcs = desc_cs;
		for (uint32_t i = 0; i != ASIZE(desc_cs); i++, pcs++)
			desc_ac.wTotalLength += pcs->bLength;
		// Clock selectors
		desc_ac.wTotalLength += desc_cx_in.bLength;
		// Input terminals
		const desc_it_t *pit = desc_it;
		for (uint32_t i = 0; i != ASIZE(desc_it); i++, pit++)
			desc_ac.wTotalLength += pit->bLength;
		// Feature units
		desc_ac.wTotalLength += desc_fu_out.bLength;
		// Output terminals
		const desc_ot_t *pot = desc_ot;
		for (uint32_t i = 0; i != ASIZE(desc_ot); i++, pot++)
			desc_ac.wTotalLength += pot->bLength;
	}
	// Audio interface association
	uint32_t s = usb_desc_add_string(usb, 0, LANG_EN_US, "USB Audio");
	usb_desc_add_interface_association(usb, 2u, AUDIO_FUNCTION,
					   FUNCTION_SUBCLASS_UNDEFINED, AF_VERSION_02_00, s);
	// Audio control interface
	usb_desc_add_interface(usb, 0u, 0u, AUDIO, AUDIOCONTROL, IP_VERSION_02_00, 0u);
	usb_desc_add(usb, &desc_ac, desc_ac.bLength);
	// Clock sources
	const desc_cs_t *pcs = desc_cs;
	for (uint32_t i = 0; i != ASIZE(desc_cs); i++, pcs++)
		usb_desc_add(usb, pcs, pcs->bLength);
	// Clock selectors
	usb_desc_add(usb, &desc_cx_in, desc_cx_in.bLength);
	// Input terminals
	const desc_it_t *pit = desc_it;
	for (uint32_t i = 0; i != ASIZE(desc_it); i++, pit++)
		usb_desc_add(usb, pit, pit->bLength);
	// Feature units
	usb_desc_add(usb, &desc_fu_out, desc_fu_out.bLength);
	// Output terminals
	const desc_ot_t *pot = desc_ot;
	for (uint32_t i = 0; i != ASIZE(desc_ot); i++, pot++)
		usb_desc_add(usb, pot, pot->bLength);
}

static void usbif_ac_setup_class(usb_t *usb, void *data, uint32_t ep, setup_t pkt)
{
	switch (pkt.bmRequestType & DIR_Msk) {
	case DIR_D2H:
		switch (pkt.bRequest) {
		case CUR:
		case RANGE:
			usb_get(usb, data, ep, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	case DIR_H2D:
		switch (pkt.bRequest) {
		case CUR:
		case RANGE:
			usb_set(usb, data, ep, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	}
}

static void usbif_as_config(usb_t *usb, void *pdata)
{
	data_t *data = (data_t *)pdata;
	epdata_t *epdata = calloc(1u, sizeof(epdata_t));
	if (!epdata)
		fatal();
	epdata->data[0] = malloc(EPOUT_MAX_SIZE);
	epdata->data[1] = malloc(EPOUT_MAX_SIZE);
	if (!epdata->data[0] || !epdata->data[1])
		fatal();

	// Register endpoints
	const epout_t epout = {
		.data = epdata,
		.init = &epout_init,
		.halt = &epout_halt,
		.xfr_cplt = &epout_xfr_cplt,
	};
	usb_ep_register(usb, 0, 0, &epout, &data->ep_out);

	// Audio streaming interface

	// Alternate setting 0, zero-bandwidth
	usb_desc_add_interface(usb, 0u, 0u, AUDIO, AUDIOSTREAMING, IP_VERSION_02_00, 0u);

	// Alternate setting 1, operational
	usb_desc_add_interface(usb, 1u, 1u, AUDIO, AUDIOSTREAMING, IP_VERSION_02_00, 0u);
	usb_desc_add(usb, &desc_as[0], desc_as[0].bLength);
	usb_desc_add(usb, &desc_pcm[0], desc_pcm[0].bLength);

	// Endpoint descriptor
	usb_desc_add_endpoint(usb, EP_DIR_OUT | data->ep_out,
			      EP_ISOCHRONOUS | EP_ISO_SYNC | EP_ISO_DATA,
			      EPOUT_MAX_SIZE, 1u);
	usb_desc_add(usb, &desc_ep[0], desc_ep[0].bLength);
}

static void usbif_as_enable(usb_t *usb, void *data)
{
	//data_t *p = (data_t *)data;
	//epout_halt(usb, p->ep_out, 0);
}

static void usbif_as_disable(usb_t *usb, void *data)
{
	data_t *p = (data_t *)data;
	epout_halt(usb, p->ep_out, 1);
}

static void usbif_as_setup_std(usb_t *usb, void *pdata, uint32_t ep, setup_t pkt)
{
	data_t *data = pdata;
	switch (pkt.bmRequestType & DIR_H2D) {
	case DIR_H2D:
		switch (pkt.bRequest) {
		case SET_INTERFACE:
			switch (pkt.wValue) {
			case 0:
				epout_halt(usb, data->ep_out, 1);
				usb_ep_in_transfer(usb->base, ep, 0, 0);
				break;
			case 1:
				epout_halt(usb, data->ep_out, 0);
				usb_ep_in_transfer(usb->base, ep, 0, 0);
				break;
			default:
				usb_ep_in_stall(usb->base, ep);
				dbgbkpt();
			}
			dbgprintf(ESC_BLUE "USB Audio setting %x\n", pkt.wValue);
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

void usb_audio2_init(usb_t *usb)
{
	data_t *data = calloc(1u, sizeof(data_t));
	if (!data)
		fatal();
	data->clk.freq[0] = 192000;
	data->clk.range[0].min = 192000;
	data->clk.range[0].max = 192000;
	data->clk.range[0].res = 0;
	for (int i = 0; i != CHANNELS; i++) {
		data->fu.mute[i] = 0x00;
		data->fu.vol[i] = 0x0000;
		data->fu.range[i].min = audio_vol_min();
		data->fu.range[i].max = audio_vol_max();
		data->fu.range[i].res = audio_vol_res();
	}
	// Audio control interface
	const usb_if_t usbif_ac = {
		.data = data,
		.config = &usbif_ac_config,
		.setup_class = &usbif_ac_setup_class,
	};
	usb_interface_register(usb, &usbif_ac);
	// Audio streaming interface
	const usb_if_t usbif_as = {
		.data = data,
		.config = &usbif_as_config,
		.enable = &usbif_as_enable,
		.disable = &usbif_as_disable,
		.setup_std = &usbif_as_setup_std,
	};
	usb_interface_register(usb, &usbif_as);
}
