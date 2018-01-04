#include <malloc.h>
#include <string.h>
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

#define MSC_IN_MAX_SIZE		1024u
#define MSC_OUT_MAX_SIZE	1024u
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
	int ep_in, ep_out;
	scsi_t *scsi;
	scsi_state_t scsi_state;
	volatile uint32_t buf_size[2];
	uint8_t lun_max, outbuf;
	union {
		cbw_t cbw;
		uint8_t raw[MSC_OUT_MAX_SIZE];
	} buf[2] ALIGN(16);
	union {
		csw_t csw;
		uint8_t raw[MSC_IN_MAX_SIZE];
	} inbuf ALIGN(4);
} usb_msc_t;

static usb_msc_t msc ALIGN(16) SECTION(.dtcm);

static void epin_init(usb_t *usb, uint32_t n)
{
	uint32_t size = MSC_IN_MAX_SIZE, addr = usb_ram_alloc(usb, &size);
	usb->base->DIEPTXF[n - 1] = DIEPTXF(addr, size);
	// Unmask interrupts
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
	ep->DIEPINT = 0;
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	dev->DAINTMSK |= DAINTMSK_IN(n);
	// Configure endpoint
	ep->DIEPCTL = USB_OTG_DIEPCTL_USBAEP_Msk | EP_TYP_BULK |
			(n << USB_OTG_DIEPCTL_TXFNUM_Pos) |
			(MSC_IN_MAX_SIZE << USB_OTG_DIEPCTL_MPSIZ_Pos);
	// Initialise buffers
	usb_msc_t *data = (usb_msc_t *)usb->epin[n].data;
	csw_t *csw = &data->inbuf.csw;
	csw->dCSWSignature = 0x53425355;
}

static void epout_swap(usb_t *usb, uint32_t n)
{
	// Receive packets
	usb_msc_t *data = (usb_msc_t *)usb->epout[n].data;
	usb_ep_out_transfer(usb->base, n, &data->buf[data->outbuf], 0u, MSC_OUT_MAX_PKT, MSC_OUT_MAX_SIZE);
	data->outbuf = !data->outbuf;
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
	epout_swap(usb, n);
}

static void epout_xfr_cplt(usb_t *usb, uint32_t n)
{
	usb_msc_t *data = (usb_msc_t *)usb->epout[n].data;
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	// Calculate packet size
	uint32_t siz = ep->DOEPTSIZ;
	uint32_t pkt_cnt = MSC_OUT_MAX_PKT - FIELD(siz, USB_OTG_DOEPTSIZ_PKTCNT);
	uint32_t size = MSC_OUT_MAX_SIZE - FIELD(siz, USB_OTG_DOEPTSIZ_XFRSIZ);
	// Enqueue packets
	if (pkt_cnt != 1u)
		dbgbkpt();
	data->buf_size[!data->outbuf] = size;
	// Check alternative buffer availability
	if (data->buf_size[data->outbuf])
		return;
	// Receive packets
	epout_swap(usb, n);
}

static void usbif_config(usb_t *usb, void *pdata)
{
	usb_msc_t *data = (usb_msc_t *)pdata;
	// Register endpoints
	const epin_t epin = {
		.data = data,
		.init = &epin_init,
		//.halt = &epin_halt,
		//.xfr_cplt = &epin_xfr_cplt,
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
	usb_msc_t *data = &msc;
	memset(data, 0, sizeof(usb_msc_t));
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

void usb_msc_process(usb_t *usb, usb_msc_t *msc)
{
	if (!msc || !msc->buf_size[msc->outbuf])
		return;

	cbw_t *cbw = &msc->buf[msc->outbuf].cbw;
	csw_t *csw = &msc->inbuf.csw;

	if ((msc->scsi_state & ~SCSIBusy) == SCSIWrite) {
		uint32_t size = msc->buf_size[msc->outbuf];
		msc->scsi_state = scsi_data(msc->scsi,
					    msc->buf[msc->outbuf].raw, size);
		// Check with SCSI again if currently busy
		if (msc->scsi_state & SCSIBusy)
			return;
		__disable_irq();
		// Mark buffer as available
		msc->buf_size[msc->outbuf] = 0;
		// Check alternative buffer status
		if (msc->buf_size[!msc->outbuf])
			epout_swap(usb, msc->ep_out);
		__enable_irq();
		// Update Command Status Wrapper (CSW)
		csw->dCSWDataResidue -= size;
		// Send Command Status Wrapper (CSW) after transfer finished
		if (msc->scsi_state != SCSIWrite)
			goto s_csw;
		return;
	}

	// Command block wrapper (CBW) processing
	if (cbw->dCBWSignature != 0x43425355) {
		// TODO: Stall bulk pipes
		dbgbkpt();
		return;
	}

	int dir = cbw->bmCBWFlags & CBW_DIR_Msk;
	if (cbw->bCBWLUN > msc->lun_max) {
		dbgbkpt();
		return;
	}
	if (cbw->bCBWCBLength < 1u || cbw->bCBWCBLength > 16u) {
		dbgbkpt();
		return;
	}

	// Process SCSI CBW
	scsi_ret_t ret = scsi_cmd(msc->scsi, cbw->CBWCB, cbw->bCBWCBLength);
	__disable_irq();
	msc->buf_size[msc->outbuf] = 0;
	// Check alternative buffer status
	if (msc->buf_size[!msc->outbuf])
		epout_swap(usb, msc->ep_out);
	__enable_irq();
	// Data transfer
	if (dir == CBW_DIR_IN) {
		if (ret.length > cbw->dCBWDataTransferLength)
			ret.length = cbw->dCBWDataTransferLength;
		usb_ep_in_transfer(usb->base, msc->ep_in, ret.p, ret.length);
	}
	msc->scsi_state = ret.state;

	// Prepare CSW
	csw->dCSWDataResidue = cbw->dCBWDataTransferLength - ret.length;
	csw->dCSWTag = cbw->dCBWTag;
s_csw:	csw->bCSWStatus = msc->scsi_state == SCSIFailure;
	// Send CSW after transfer finished
	if (msc->scsi_state == SCSIGood || msc->scsi_state == SCSIFailure)
		usb_ep_in_transfer(usb->base, msc->ep_in, csw, 13);
}
