#include <malloc.h>
#include <string.h>
#include "../../debug.h"
#include "../../peripheral/audio.h"
#include "../usb.h"
#include "../usb_structs.h"
#include "../usb_setup.h"
#include "../usb_ram.h"
#include "../usb_ep.h"
#include "usb_audio2.h"
#include "usb_audio2_desc.h"

#define EPOUT_MAX_SIZE	1024u
#define CHANNELS	(1u + 2u)

// Parameter block layout types
typedef uint8_t layout1_cur_t;

typedef struct PACKED {
	uint8_t min, max, res;
} layout1_range_t;

typedef uint16_t layout2_cur_t;

typedef struct PACKED {
	uint16_t min, max, res;
} layout2_range_t;

typedef uint32_t layout3_cur_t;

typedef struct PACKED {
	uint32_t min, max, res;
} layout3_range_t;

// Private data types
typedef struct {
	struct {
		layout3_cur_t freq[1];
		layout3_range_t range[1];
	} cs;
	struct {
		layout2_cur_t vol[CHANNELS];
		layout2_range_t range[CHANNELS];
		layout1_cur_t mute[CHANNELS];
	} fu;
	struct PACKED {
		union {
			struct {
				uint16_t wNumSubRanges;
				uint8_t range[];
			};
			uint8_t raw[16];
		};
	} buf;
	uint8_t ac[1], as[1];
	int ep_out;
} data_t;

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
	ep->DOEPTSIZ = (1u << USB_OTG_DOEPTSIZ_PKTCNT_Pos) | EPOUT_MAX_SIZE;
	// Check frame parity
	uint32_t fn = FIELD(dev->DSTS, USB_OTG_DSTS_FNSOF);
	fn = (fn & 1) ? USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Msk : USB_OTG_DOEPCTL_SODDFRM;
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
	if (!(ctl & USB_OTG_DOEPCTL_EPENA_Msk) != !halt)
		return;
	if (halt) {
		DOEPCTL_SET(ep->DOEPCTL, USB_OTG_DOEPCTL_EPDIS);
		//while (ep->DOEPCTL & USB_OTG_DOEPCTL_EPENA_Msk);
		dbgprintf(ESC_BLUE "USB Audio out disabled\n");
	} else {
		epout_recv(usb, n);
		dbgprintf(ESC_BLUE "USB Audio out enabled\n");
	}
}

static void epout_xfr_cplt(usb_t *usb, uint32_t n)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	uint32_t siz = ep->DOEPTSIZ;
	epout_recv(usb, n);
	uint32_t pktcnt = 1u - FIELD(siz, USB_OTG_DOEPTSIZ_PKTCNT);
	uint32_t size = EPOUT_MAX_SIZE - FIELD(siz, USB_OTG_DOEPTSIZ_PKTCNT);
	//dbgprintf(ESC_BLUE "<X|%lx|%lx>", pktcnt, size);
	dbgprintf(".");
}

static inline uint16_t layout_cur_get(data_t *data, int type, void *p)
{
	const unsigned int msize = 1u << (type - 1u);
	memcpy(data->buf.raw, p, msize);
#if 0
	uint32_t v = 0;
	memcpy(&v, p, msize);
	dbgprintf("%lx|", v);
#endif
	return msize;
}

static inline uint32_t layout_cur(int type, void *p)
{
	uint32_t v = 0;
	const unsigned int msize = 1u << (type - 1u);
	memcpy(&v, p, msize);
	dbgprintf("%lx|", v);
	return v;
}

static inline uint16_t layout_range_get(data_t *data, int type, void *p, uint16_t cnt)
{
	const unsigned int msize = 1u << (type - 1u);
	const uint16_t size = 3u * msize * cnt;
	data->buf.wNumSubRanges = cnt;
	memcpy(data->buf.range, p, size);
#if 0
	while (cnt--) {
		for (int i = 0; i != 3; i++) {
			uint32_t v = 0;
			memcpy(&v, p, msize);
			dbgprintf("%lx|", v);
			p += msize;
		}
	}
#endif
	return 2u + size;
}

static inline desc_t cs_get(data_t *data, setup_t pkt)
{
	desc_t desc = {0, data->buf.raw};
	uint8_t cs = pkt.bType, cn = pkt.bIndex;
	dbgprintf(ESC_GREEN "(CS_");
	switch (cs) {
	case CS_SAM_FREQ_CONTROL:
		// Layout 3 parameter block
		if (cn != 0) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("f");
			desc.size = layout_cur_get(data, 3, &data->cs.freq[0]);
			break;
		case RANGE:
			dbgprintf("fr");
			desc.size = layout_range_get(data, 3, &data->cs.range[0], 1u);
			break;
		default:
			dbgbkpt();
		}
		break;
	default:
		dbgbkpt();
	}
	dbgprintf("%u/%lu@%u)", cn, desc.size, pkt.wLength);
	return desc;
}

static inline desc_t fu_get(data_t *data, setup_t pkt)
{
	desc_t desc = {0, data->buf.raw};
	uint8_t cs = pkt.bType, cn = pkt.bIndex;
	dbgprintf(ESC_GREEN "(FU_");
	switch (cs) {
	case FU_MUTE_CONTROL:
		// Layout 1 parameter block
		if (cn >= ASIZE(data->fu.mute)) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("m");
			desc.size = layout_cur_get(data, 1, &data->fu.mute[cn]);
			break;
		default:
			dbgbkpt();
		}
		break;
	case FU_VOLUME_CONTROL:
		// Layout 2 parameter block
		if (cn >= ASIZE(data->fu.vol)) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("v");
			desc.size = layout_cur_get(data, 2, &data->fu.vol[cn]);
			break;
		case RANGE:
			dbgprintf("vr");
			desc.size = layout_range_get(data, 2, &data->fu.range[cn], 1u);
			break;
		default:
			dbgbkpt();
		}
		break;
	default:
		dbgbkpt();
	}
	dbgprintf("%u/%lu@%u)", cn, desc.size, pkt.wLength);
	return desc;
}

static inline int fu_set(data_t *data, setup_t pkt, void *buf)
{
	uint16_t v;
	uint8_t cs = pkt.bType, cn = pkt.bIndex;
	dbgprintf(ESC_RED "(FU_");
	switch (cs) {
	case FU_MUTE_CONTROL:
		// Layout 1 parameter block
		if (cn >= ASIZE(data->fu.mute)) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("M");
			data->fu.mute[cn] = layout_cur(1, buf);
			dbgprintf("%u)", cn);
			return 1;
		default:
			dbgbkpt();
		}
	case FU_VOLUME_CONTROL:
		// Layout 2 parameter block
		if (cn >= ASIZE(data->fu.vol)) {
			dbgbkpt();
			break;
		}
		switch (pkt.bRequest) {
		case CUR:
			dbgprintf("V");
			data->fu.vol[cn] = layout_cur(2, buf);
			dbgprintf("%u)", cn);
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
	case CS_USB:
		desc = cs_get(data, pkt);
		break;
	case FU_Out:
		desc = fu_get(data, pkt);
		break;
	}
	if (desc.size == 0) {
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	} else {
		if (desc.size > pkt.wLength)
			desc.size = pkt.wLength;
		usb_ep_in_transfer(usb->base, ep, desc.p, desc.size);
	}
}

static inline void usb_set(usb_t *usb, data_t *data, uint32_t ep, setup_t pkt)
{
	switch (pkt.bEntityID) {
	case FU_Out:
		if (fu_set(data, pkt, usb->setup_buf))
			usb_ep_in_transfer(usb->base, ep, 0, 0);
		else
			usb_ep_in_stall(usb->base, ep);
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	}
}

static void usbif_ac_config(usb_t *usb, void *pdata)
{
	data_t *data = (data_t *)pdata;
	if (!desc_ac.wTotalLength) {
		desc_ac.wTotalLength = desc_ac.bLength;
		// Clock sources
		const desc_csd_t *pcs = desc_csd;
		for (uint32_t i = 0; i != ASIZE(desc_csd); i++, pcs++)
			desc_ac.wTotalLength += pcs->bLength;
		// Input terminals
		const desc_itd_t *pit = desc_itd;
		for (uint32_t i = 0; i != ASIZE(desc_itd); i++, pit++)
			desc_ac.wTotalLength += pit->bLength;
		// Feature units
		desc_ac.wTotalLength += desc_fu_out.bLength;
		// Output terminals
		const desc_otd_t *pot = desc_otd;
		for (uint32_t i = 0; i != ASIZE(desc_otd); i++, pot++)
			desc_ac.wTotalLength += pot->bLength;
	}
	// Audio interface association
	uint32_t s = usb_desc_add_string(usb, 0, LANG_EN_US, "USB Audio");
	usb_desc_add_interface_association(usb, 2u, AUDIO_FUNCTION,
					   FUNCTION_SUBCLASS_UNDEFINED, AF_VERSION_02_00, s);
	// Audio control interface
	data->ac[0] = usb_desc_add_interface(usb, 0u, 0u, AUDIO, AUDIOCONTROL, IP_VERSION_02_00, 0u);
	usb_desc_add(usb, &desc_ac, desc_ac.bLength);
	// Clock sources
	const desc_csd_t *pcs = desc_csd;
	for (uint32_t i = 0; i != ASIZE(desc_csd); i++, pcs++)
		usb_desc_add(usb, pcs, pcs->bLength);
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
	epdata->data[0] = malloc(sizeof(EPOUT_MAX_SIZE));
	epdata->data[1] = malloc(sizeof(EPOUT_MAX_SIZE));
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
	data->as[0] = usb_desc_add_interface(usb, 0u, 0u, AUDIO, AUDIOSTREAMING, IP_VERSION_02_00, 0u);

	// Alternate setting 1, operational
	usb_desc_add_interface(usb, 1u, 1u, AUDIO, AUDIOSTREAMING, IP_VERSION_02_00, 0u);
	usb_desc_add(usb, &desc_as[0], desc_as[0].bLength);
	usb_desc_add(usb, &desc_pcm[0], desc_pcm[0].bLength);

	// Endpoint descriptor
	usb_desc_add_endpoint(usb, EP_DIR_OUT | data->ep_out,
			      EP_ISOCHRONOUS | EP_ISO_SYNC | EP_ISO_DATA,
			      EPOUT_MAX_SIZE, 1u);
	usb_desc_add(usb, &desc_ep[0], desc_ep[0].bLength);

	// Interface association descriptor
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

void usb_audio2_init(usb_t *usb)
{
	data_t *data = calloc(1u, sizeof(data_t));
	data->cs.freq[0] = 44100;
	data->cs.range[0].min = 44100;
	data->cs.range[0].max = 44100;
	data->cs.range[0].res = 0;
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
