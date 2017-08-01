#include <malloc.h>
#include <string.h>
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
#define CHANNELS	(1u + 2u)

typedef struct data_t
{
	int ep_out;
	struct {
		uint8_t mute[CHANNELS];
		uint16_t vol[CHANNELS];
		uint16_t vol_min[CHANNELS], vol_max[CHANNELS], vol_res[CHANNELS];
	} fu_out;
	uint8_t buf[8];
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
		dbgprintf(ESC_BLUE "USB Audio out disabled\n");
		DOEPCTL_SET(ep->DOEPCTL, USB_OTG_DOEPCTL_EPDIS_Msk);
		//while (ep->DOEPCTL & USB_OTG_DOEPCTL_EPENA_Msk);
		return;
	}
	dbgprintf(ESC_BLUE "USB Audio out enabled\n");
	// Enable endpoint
	uint32_t fn = FIELD(dev->DSTS, USB_OTG_DSTS_FNSOF);
	fn = 0;///*(fn & 1) ? 0 :*/ USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Msk;
	DOEPCTL_SET(ep->DOEPCTL, USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK | fn);
}

static void epout_xfr_cplt(usb_t *usb, uint32_t n)
{
	dbgbkpt();
}

static inline desc_t fu_out_get(data_t *data, setup_t pkt)
{
	desc_t desc = {0, 0};
	uint32_t size = 0, stride = 0;
	uint8_t cs = pkt.bType, cn = pkt.bIndex;
	switch (cs) {
	case MUTE_CONTROL:
		stride = 1;
		switch (pkt.bRequest) {
		case GET_CUR:
			dbgprintf(ESC_GREEN "(m");
			desc.p = data->fu_out.mute;
			size = ASIZE(data->fu_out.mute);
			break;
		default:
			dbgbkpt();
		}
		break;
	case VOLUME_CONTROL:
		stride = 2;
		switch (pkt.bRequest) {
		case GET_RES:
			dbgprintf(ESC_GREEN "(vr");
			desc.p = data->fu_out.vol_res;
			size = ASIZE(data->fu_out.vol_res);
			break;
		case GET_MAX:
			dbgprintf(ESC_GREEN "(vM");
			desc.p = data->fu_out.vol_max;
			size = ASIZE(data->fu_out.vol_max);
			break;
		case GET_MIN:
			dbgprintf(ESC_GREEN "(vm");
			desc.p = data->fu_out.vol_min;
			size = ASIZE(data->fu_out.vol_min);
			break;
		case GET_CUR:
			dbgprintf(ESC_GREEN "(vc");
			desc.p = data->fu_out.vol;
			size = ASIZE(data->fu_out.vol);
			break;
		default:
			dbgbkpt();
		}
		break;
	default:
		dbgbkpt();
	}
	if (desc.p) {
		if (cn == 0xff) {
			desc.size = MIN(pkt.wLength, size * stride);
			dbgbkpt();
		} else if (cn < size) {
			desc.p += cn * stride;
			desc.size = MIN(pkt.wLength, stride);
			uint16_t v;
			memcpy(&v, desc.p, stride);
			dbgprintf("%u/%x)", cn, v);
		} else
			dbgbkpt();
		memcpy(data->buf, desc.p, desc.size);
		desc.p = data->buf;
	}
	return desc;
}

static inline int fu_out_set(data_t *data, setup_t pkt, void *buf)
{
	uint16_t v;
	uint8_t cs = pkt.bType, cn = pkt.bIndex;
	switch (cs) {
	case MUTE_CONTROL:
		v = *(uint8_t *)buf;
		switch (pkt.bRequest) {
		case SET_CUR:
			dbgprintf(ESC_RED "(M%u/%x)", cn, v);
			return 1;
		default:
			dbgbkpt();
		}
		break;
	case VOLUME_CONTROL:
		v = *(uint16_t *)buf;
		switch (pkt.bRequest) {
		case SET_CUR:
			dbgprintf(ESC_RED "(Vc%u/%x)", cn, v);
			if (cn >= ASIZE(data->fu_out.vol))
				dbgbkpt();
			else
				data->fu_out.vol[cn] = v;
			return 1;
		case SET_MIN:
			dbgprintf(ESC_RED "(Vm%u/%x)", cn, v);
			if (v != audio_vol_min())
				return 0;
			return 1;
		case SET_MAX:
			dbgprintf(ESC_RED "(VM%u/%x)", cn, v);
			if (v != audio_vol_max())
				return 0;
			return 1;
		case SET_RES:
			dbgprintf(ESC_RED "(Vr%u/%x)", cn, v);
			if (v != audio_vol_res())
				return 0;
			return 1;
		default:
			dbgbkpt();
		}
		break;
	default:
		dbgbkpt();
	}
	return 0;
}

static inline void usb_get(usb_t *usb, data_t *data, uint32_t ep, setup_t pkt)
{
	desc_t desc = {0, 0};
	switch (pkt.bEntityID) {
	case FU_Out:
		desc = fu_out_get(data, pkt);
		break;
	}
	if (desc.size == 0) {
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	} else
		usb_ep_in_transfer(usb->base, ep, desc.p, desc.size);
}

static inline void usb_set(usb_t *usb, data_t *data, uint32_t ep, setup_t pkt)
{
	switch (pkt.bEntityID) {
	case FU_Out:
		if (fu_out_set(data, pkt, usb->setup_buf))
			usb_ep_in_transfer(usb->base, ep, 0, 0);
		else
			usb_ep_in_stall(usb->base, ep);
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	}
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
		desc_ac.wTotalLength += desc_fu_out.bLength;
	}
	// Audio control interface
	uint32_t s = usb_desc_add_string(usb, 0, LANG_EN_US, "USB Audio");
	uint8_t iface = usb_desc_add_interface(usb, 0u, 0u, AUDIO, AUDIOCONTROL, PR_PROTOCOL_UNDEFINED, s);
	desc_ac.baInterfaceNr[0] = iface + 1;
	usb_desc_add(usb, &desc_ac, desc_ac.bLength);
	// Input terminals
	const desc_itd_t *pit = desc_itd;
	for (uint32_t i = 0; i != ASIZE(desc_itd); i++, pit++)
		usb_desc_add(usb, pit, pit->bLength);
	// Feature units
	usb_desc_add(usb, &desc_fu_out, desc_fu_out.bLength);
	// Output terminals
	const desc_otd_t *pot = desc_otd;
	for (uint32_t i = 0; i != ASIZE(desc_otd); i++, pot++)
		usb_desc_add(usb, pot, pot->bLength);
}

static void usbif_ac_setup_class(usb_t *usb, void *data, uint32_t ep, setup_t pkt)
{
	switch (pkt.bmRequestType & SETUP_TYPE_DIR_Msk) {
	case SETUP_TYPE_DIR_D2H:
		switch (pkt.bRequest) {
		case GET_RES:
		case GET_MIN:
		case GET_MAX:
		case GET_CUR:
			usb_get(usb, data, ep, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	case SETUP_TYPE_DIR_H2D:
		switch (pkt.bRequest) {
		case SET_RES:
		case SET_MIN:
		case SET_MAX:
		case SET_CUR:
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
	usb_desc_add_interface(usb, 0u, 0u, AUDIO, AUDIOSTREAMING, PR_PROTOCOL_UNDEFINED, 0u);

	// Alternate setting 1, operational
	usb_desc_add_interface(usb, 1u, 1u, AUDIO, AUDIOSTREAMING, PR_PROTOCOL_UNDEFINED, 0u);
	usb_desc_add(usb, &desc_as[0], desc_as[0].bLength);
	usb_desc_add(usb, &desc_pcm[2], desc_pcm[2].bLength);

	// Endpoint descriptor
	usb_desc_add_endpoint(usb, EP_DIR_OUT | p->ep_out,
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

static void usbif_as_setup_std(usb_t *usb, void *p, uint32_t ep, setup_t pkt)
{
	data_t *data = p;
	switch (pkt.bmRequestType & SETUP_TYPE_DIR_H2D) {
	case SETUP_TYPE_DIR_H2D:
		switch (pkt.bRequest) {
		case SETUP_REQ_SET_INTERFACE:
			switch (pkt.wValue) {
			case 0:
				audio_out_enable(0);
				epout_halt(usb, data->ep_out, 1);
				usb_ep_in_transfer(usb->base, ep, 0, 0);
				break;
			case 1:
				audio_out_enable(1);
				epout_halt(usb, data->ep_out, 0);
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
	data_t *data = calloc(1u, sizeof(data_t));
	for (int i = 0; i != CHANNELS; i++) {
		data->fu_out.vol[i] = 0x0000;
		data->fu_out.vol_min[i] = audio_vol_min();
		data->fu_out.vol_max[i] = audio_vol_max();
		data->fu_out.vol_res[i] = audio_vol_res();
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
