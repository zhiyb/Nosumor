#include <malloc.h>
#include <debug.h>
#include "../usb.h"
#include "../usb_structs.h"
#include "../usb_ram.h"
#include "../usb_ep.h"
#include "../usb_desc.h"
#include "../usb_macros.h"
#include "usb_msc.h"
#include "usb_msc_defs.h"
#include "scsi.h"

#define MSC_IN_MAX_SIZE		64u
#define MSC_OUT_MAX_SIZE	64u
#define MSC_OUT_MAX_PKT		1u

#define CBW_DIR_Msk	0x80
#define CBW_DIR_IN	0x80
#define CBW_DIR_OUT	0x00

typedef struct PACKED cbw_t {
	uint32_t dCBWSignature;
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
} cbw_t;

typedef struct PACKED csw_t {
	uint32_t dCSWSignature;
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
} csw_t;

typedef struct usb_msc_t {
	scsi_t *scsi;
	int ep_in, ep_out;
	union {
		cbw_t cbw;
		uint8_t raw[MSC_OUT_MAX_SIZE];
	} outbuf, buf[MSC_OUT_MAX_PKT];
	union {
		csw_t csw;
		uint8_t raw[MSC_IN_MAX_SIZE];
	} inbuf;
	uint8_t lun_max;
} usb_msc_t;

static void epin_init(usb_t *usb, uint32_t n)
{
	uint32_t size = MSC_IN_MAX_SIZE, addr = usb_ram_alloc(usb, &size);
	usb->base->DIEPTXF[n - 1] = DIEPTXF(addr, size);
	// Unmask interrupts
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
	ep->DIEPINT = USB_OTG_DIEPINT_XFRC_Msk;
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	dev->DAINTMSK |= DAINTMSK_IN(n);
	// Configure endpoint
	ep->DIEPCTL = EP_TYP_BULK | (n << USB_OTG_DIEPCTL_TXFNUM_Pos) |
			(MSC_IN_MAX_SIZE << USB_OTG_DIEPCTL_MPSIZ_Pos);
	// Initialise buffers
	usb_msc_t *data = (usb_msc_t *)usb->epout[n].data;
	csw_t *csw = &data->inbuf.csw;
	csw->dCSWSignature = 0x53425355;
}

static void epin_xfr_cplt(usb_t *usb, uint32_t n)
{
	usb_msc_t *data = (usb_msc_t *)usb->epin[n].data;
	dbgbkpt();
}

static void epout_init(usb_t *usb, uint32_t n)
{
	// Set endpoint type
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	ep->DOEPCTL = USB_OTG_DOEPCTL_USBAEP_Msk | EP_TYP_BULK | MSC_OUT_MAX_SIZE;
	// Clear interrupts
	ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk;
	// Unmask interrupts
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	dev->DAINTMSK |= DAINTMSK_OUT(n);
	// Receive packets
	usb_msc_t *data = (usb_msc_t *)usb->epout[n].data;
	usb_ep_out_transfer(usb->base, n, data->buf, 0u, MSC_OUT_MAX_PKT, MSC_OUT_MAX_SIZE);
}

static void bulk_cbw(usb_t *usb, int n)
{
	usb_msc_t *data = (usb_msc_t *)usb->epout[n].data;
	cbw_t *cbw = &data->outbuf.cbw;
	csw_t *csw = &data->inbuf.csw;
	if (cbw->dCBWSignature != 0x43425355)
		dbgbkpt();
	csw->dCSWTag = cbw->dCBWTag;
	int dir = cbw->bmCBWFlags & CBW_DIR_Msk;
	if (cbw->bCBWLUN > data->lun_max)
		dbgbkpt();
	if (cbw->bCBWCBLength < 1u || cbw->bCBWCBLength > 16u)
		dbgbkpt();
	dbgprintf(ESC_YELLOW "<CBW|%u|", !!dir);
	uint8_t *p = cbw->CBWCB;
	for (int i = cbw->bCBWCBLength; i != 0; i--)
		dbgprintf("%02x,", *p++);
	dbgprintf(">");
	scsi_cmd(data->scsi, cbw->CBWCB, cbw->bCBWCBLength);
}

static void epout_xfr_cplt(usb_t *usb, uint32_t n)
{
	usb_msc_t *data = (usb_msc_t *)usb->epout[n].data;
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	// Calculate packet size
	uint32_t siz = ep->DOEPTSIZ;
	uint32_t pkt_cnt = MSC_OUT_MAX_PKT - FIELD(siz, USB_OTG_DOEPTSIZ_PKTCNT);
	uint32_t size = MSC_OUT_MAX_SIZE - FIELD(siz, USB_OTG_DOEPTSIZ_XFRSIZ);
	// Copy packet
	if (pkt_cnt != 1u)
		dbgbkpt();
	if (size != 31u)
		dbgbkpt();
	memcpy(&data->outbuf, data->buf, size);
	// Receive new packets
	usb_ep_out_transfer(usb->base, n, data->buf, 0u, MSC_OUT_MAX_PKT, MSC_OUT_MAX_SIZE);
	// Command block wrapper (CBW) processing
	bulk_cbw(usb, n);
}

static void usbif_config(usb_t *usb, void *pdata)
{
	usb_msc_t *data = (usb_msc_t *)pdata;
	// Register endpoints
	const epin_t epin = {
		.data = data,
		.init = &epin_init,
		//.halt = &epin_halt,
		.xfr_cplt = &epin_xfr_cplt,
	};
	const epout_t epout = {
		.data = data,
		.init = &epout_init,
		//.halt = &epout_halt,
		.xfr_cplt = &epout_xfr_cplt,
	};
	usb_ep_register(usb, &epin, &data->ep_in, &epout, &data->ep_out);

	uint32_t s = usb_desc_add_string(usb, 0, LANG_EN_US, "USB Mass Storage");
	usb_desc_add_interface(usb, 0u, 2u, MASS_STORAGE, SCSI_TRANSPARENT, BBB, s);
	usb_desc_add_endpoint(usb, EP_DIR_IN | data->ep_in,
			      EP_BULK, MSC_IN_MAX_SIZE, 0u);
	usb_desc_add_endpoint(usb, EP_DIR_OUT | data->ep_out,
			      EP_BULK, MSC_OUT_MAX_SIZE, 0u);
}

static void usbif_setup_class(usb_t *usb, void *pdata, uint32_t ep, setup_t pkt)
{
	usb_msc_t *data = (usb_msc_t *)pdata;
	switch (pkt.bmRequestType & DIR_Msk) {
	case DIR_D2H:
		switch (pkt.bRequest) {
		case GML:
			usb_ep_in_transfer(usb->base, ep, &data->lun_max, 1u);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	case DIR_H2D:
		switch (pkt.bRequest) {
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	}
}

usb_msc_t *usb_msc_init(usb_t *usb)
{
	usb_msc_t *data = calloc(1u, sizeof(usb_msc_t));
	if (!data)
		panic();
	data->scsi = scsi_init();
	// Audio control interface
	const usb_if_t usbif = {
		.data = data,
		.config = &usbif_config,
		.setup_class = &usbif_setup_class,
	};
	usb_interface_register(usb, &usbif);
	return data;
}
