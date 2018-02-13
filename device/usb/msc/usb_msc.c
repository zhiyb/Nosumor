#include <malloc.h>
#include <string.h>
#include <debug.h>
#include <logic/scsi.h>
#include "../usb.h"
#include "../usb_structs.h"
#include "../usb_ram.h"
#include "../usb_ep.h"
#include "../usb_desc.h"
#include "../usb_macros.h"
#include "usb_msc.h"
#include "usb_msc_defs.h"

#define MSC_IN_MAX_SIZE		((USB_OTG_DIEPTSIZ_XFRSIZ_Msk + 1ul) >> 1ul)
#define MSC_IN_PKT_SIZE		512u
#define MSC_OUT_MAX_SIZE	512u
#define MSC_OUT_MAX_PKT		1u
#define MSC_FIFO_LEVEL		8u

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

struct fifo_t {
	const void *entry[MSC_FIFO_LEVEL];
	uint32_t size[MSC_FIFO_LEVEL];
	volatile uint16_t next;
	volatile uint16_t level;
};

typedef struct usb_msc_t {
	int ep_in, ep_out;
	scsi_t **scsi;
	volatile uint32_t buf_size[2];
	uint8_t lun_max ALIGN(4);
	volatile uint8_t outbuf;
	volatile uint8_t active;
	volatile uint8_t in_active;
	const void *in_ptr;
	uint16_t in_size;
	struct fifo_t fifo;
} usb_msc_t;

static usb_msc_t msc SECTION(.dtcm);

static union {
	cbw_t cbw;
	uint8_t raw[MSC_OUT_MAX_SIZE];
} buf[2] SECTION(.dtcm) ALIGN(4);
static union {
	csw_t csw;
	uint8_t raw[0];
} inbuf SECTION(.dtcm) ALIGN(4);

static uint16_t fifo_level(struct fifo_t *fifo)
{
	return fifo->level;
}

static void fifo_push(struct fifo_t *fifo, const void *p, uint32_t size)
{
	uint16_t idx = fifo->next;
	fifo->level++;
	fifo->next = (idx + 1) & (MSC_FIFO_LEVEL - 1);
	fifo->entry[idx] = p;
	fifo->size[idx] = size;
}

static const void *fifo_pop(struct fifo_t *fifo, uint32_t *size)
{
	uint16_t idx = (fifo->next - fifo->level) & (MSC_FIFO_LEVEL - 1);
	fifo->level--;
	*size = fifo->size[idx];
	const void *p = fifo->entry[idx];
	return p;
}

static void epin_init(usb_t *usb, uint32_t n)
{
	uint32_t size = MSC_IN_PKT_SIZE + 64, addr = usb_ram_alloc(usb, &size);
	usb->base->DIEPTXF[n - 1] = DIEPTXF(addr, size);
	// Unmask interrupts
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
	ep->DIEPINT = 0;
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	dev->DAINTMSK |= DAINTMSK_IN(n);
	// Configure endpoint
	ep->DIEPCTL = USB_OTG_DIEPCTL_USBAEP_Msk | EP_TYP_BULK |
			(n << USB_OTG_DIEPCTL_TXFNUM_Pos) |
			(MSC_IN_PKT_SIZE << USB_OTG_DIEPCTL_MPSIZ_Pos);
	// Initialise buffers
	usb_msc_t *data = (usb_msc_t *)usb->epin[n].data;
	csw_t *csw = &inbuf.csw;
	csw->dCSWSignature = 0x53425355;
}

static void epin_xfr_cplt(usb_t *usb, uint32_t n)
{
	usb_msc_t *msc = (usb_msc_t *)usb->epin[n].data;
	uint8_t active = msc->in_active;
	// Skip for CSW transfers
	if (active == 0xff)
		return;
	const void *ptr = msc->in_ptr;
	uint16_t size = msc->in_size;
	// Start new transfer if data is available
	if (fifo_level(&msc->fifo)) {
		uint32_t length;
		const void *p = fifo_pop(&msc->fifo, &length);
		if (usb_ep_in_transfer(usb->base, n, p, length) != length)
			dbgbkpt();
		msc->in_ptr = p;
		msc->in_size = length;
		if (!length)
			dbgbkpt();
	} else {
		msc->in_active = 0xff;
	}
	scsi_t *scsi = *(msc->scsi + active);
	scsi_data_cplt(scsi, ptr, size);
}

static void epout_swap(usb_t *usb, uint32_t n)
{
	// Receive packets
	usb_msc_t *data = (usb_msc_t *)usb->epout[n].data;
	uint8_t outbuf = data->outbuf;
	usb_ep_out_transfer(usb->base, n, &buf[outbuf], 0u, MSC_OUT_MAX_PKT, MSC_OUT_MAX_SIZE);
	data->outbuf = !outbuf;
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

static void epout_stall(usb_t *usb, uint32_t n, int stall)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	if (stall) {
		__disable_irq();
		// Set stall bit
		DOEPCTL_SET(ep->DOEPCTL, USB_OTG_DOEPCTL_STALL_Msk);
		// Clear interrupts
		ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk;
		__enable_irq();

		// Flush buffers
		usb_msc_t *data = (usb_msc_t *)usb->epout[n].data;
		data->buf_size[0] = 0;
		data->buf_size[1] = 0;

		dbgprintf(ESC_MSG "[MSC] OUT endpoint " ESC_DISABLE "stalled\n");
	} else {
		if (!(ep->DOEPCTL & USB_OTG_DOEPCTL_STALL_Msk))
			return;

		// Clear stall bit, resume data transfer
		DOEPCTL_SET(ep->DOEPCTL, USB_OTG_DOEPCTL_EPENA_Msk |
			    USB_OTG_DOEPCTL_CNAK_Msk |
			    USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Msk);

		dbgprintf(ESC_MSG "[MSC] OUT endpoint " ESC_ENABLE "resumed\n");
	}
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
	if (pkt_cnt != 1u || size == 0) {
		dbgbkpt();
		return;
	}
	// Update buffer size
	data->buf_size[!data->outbuf] = size;
	// Check alternative buffer availability
	if (data->buf_size[data->outbuf])
		return;
	// Receive packets
	epout_swap(usb, n);
}

static void usbif_config(usb_t *usb, void *pdata)
{
	usb_msc_t *msc = (usb_msc_t *)pdata;
	// If no LUNs available
	if (msc->lun_max == 0xff)
		return;

	// Register endpoints
	const epin_t epin = {
		.data = msc,
		.init = &epin_init,
		//.halt = &epin_halt,
		.xfr_cplt = &epin_xfr_cplt,
	};
	const epout_t epout = {
		.data = msc,
		.init = &epout_init,
		.halt = &epout_stall,
		.xfr_cplt = &epout_xfr_cplt,
	};
	usb_ep_register(usb, &epin, &msc->ep_in, &epout, &msc->ep_out);

	uint32_t s = usb_desc_add_string(usb, 0, LANG_EN_US, "USB Mass Storage");
	usb_desc_add_interface(usb, 0u, 2u, MASS_STORAGE, SCSI_TRANSPARENT, BBB, s);
	usb_desc_add_endpoint(usb, EP_DIR_IN | msc->ep_in,
			      EP_BULK, MSC_IN_PKT_SIZE, 0u);
	usb_desc_add_endpoint(usb, EP_DIR_OUT | msc->ep_out,
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

static void process_cmd(usb_t *usb, usb_msc_t *msc)
{
	// Process USB packets
	uint8_t outbuf = msc->outbuf;
	if (!msc->buf_size[outbuf])
		return;

	// Command block wrapper (CBW) processing
	cbw_t *cbw = &buf[outbuf].cbw;
	if (cbw->dCBWSignature != 0x43425355) {
		// TODO: Stall bulk pipes
		dbgbkpt();
		return;
	}

	int dir = cbw->bmCBWFlags & CBW_DIR_Msk;
	uint8_t lun = cbw->bCBWLUN;
	scsi_t *scsi = *(msc->scsi + lun);
	if (cbw->bCBWCBLength < 1u || cbw->bCBWCBLength > 16u) {
		dbgbkpt();
		return;
	}

	// Process SCSI commands
	csw_t *csw = &inbuf.csw;
	scsi_ret_t ret = scsi_cmd(scsi, cbw->CBWCB, cbw->bCBWCBLength);
	// Prepare CSW
	csw->dCSWDataResidue = cbw->dCBWDataTransferLength;
	csw->dCSWTag = cbw->dCBWTag;
	// Mark buffer as available
	__disable_irq();
	if (outbuf != msc->outbuf)
		dbgbkpt();
	msc->buf_size[outbuf] = 0;
	// Check alternative buffer status
	uint32_t buf_size = msc->buf_size[!outbuf];
	__enable_irq();
	if (buf_size)
		epout_swap(usb, msc->ep_out);
	// SCSI data ready for transfer
	if (ret.state == SCSIGood || ret.state == SCSIFailure) {
		if (dir == CBW_DIR_IN) {
			if (ret.length > csw->dCSWDataResidue)
				ret.length = csw->dCSWDataResidue;
			if (usb_ep_in_transfer(usb->base, msc->ep_in,
					       ret.p, ret.length) != ret.length)
				dbgbkpt();
		} else if (csw->dCSWDataResidue) {
			// Host is sending data, stall the OUT endpoint
			epout_stall(usb, msc->ep_out, 1);
		}
	} else {
		// SCSI data transfer active
		msc->active = lun;
	}

	// Prepare CSW
	csw->dCSWDataResidue -= ret.length;
	csw->bCSWStatus = ret.state == SCSIFailure;
	// Send CSW after transfer finished
	if (ret.state == SCSIGood || ret.state == SCSIFailure)
		if (usb_ep_in_transfer(usb->base, msc->ep_in, csw, 13u) != 13u)
			dbgbkpt();
}

static void process_data(usb_t *usb, usb_msc_t *msc)
{
	csw_t *csw = &inbuf.csw;
	scsi_t *scsi = *(msc->scsi + msc->active);

	// Process SCSI initiated data packets
	scsi_ret_t ret = scsi_process(scsi, MSC_IN_PKT_SIZE, MSC_IN_MAX_SIZE);
	if (ret.length || ret.state == SCSIFailure) {
		USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
		// Disable EP IN interrupts
		dev->DAINTMSK &= ~DAINTMSK_IN(msc->ep_in);
		__ISB();
		// If the endpoint is not active, start transfer immediately
		if (msc->in_active == 0xff) {
			if (usb_ep_in_transfer(usb->base, msc->ep_in,
					       ret.p, ret.length) != ret.length)
				dbgbkpt();
			msc->in_active = msc->active;
			msc->in_ptr = ret.p;
			msc->in_size = ret.length;
		} else {
			fifo_push(&msc->fifo, ret.p, ret.length);
		}
		// Enable interrupts
		dev->DAINTMSK |= DAINTMSK_IN(msc->ep_in);
		// Update Command Status Wrapper (CSW)
		csw->dCSWDataResidue -= ret.length;
		// Send CSW after transfer finished
		if (ret.state == SCSIGood || ret.state == SCSIFailure)
			goto s_csw;
		return;
	}

	// Check if SCSI is not receiving data packets
	if ((ret.state & ~SCSIBusy) != SCSIWrite)
		return;

	// Process USB data packets
	uint8_t outbuf = msc->outbuf;
	if (!msc->buf_size[outbuf])
		return;

	uint32_t size = msc->buf_size[outbuf];
	ret.state = scsi_data(scsi, buf[outbuf].raw, size);
	// Check with SCSI again if currently busy
	if (ret.state & SCSIBusy)
		return;

	__disable_irq();
	if (outbuf != msc->outbuf)
		dbgbkpt();
	// Mark buffer as available
	msc->buf_size[outbuf] = 0;
	// Check alternative buffer status
	uint32_t buf_size = msc->buf_size[!outbuf];
	__enable_irq();
	if (buf_size)
		epout_swap(usb, msc->ep_out);

	// Update CSW
	csw->dCSWDataResidue -= size;
	// Wait for more data packets if transfer have not finished
	if (ret.state != SCSIGood && ret.state != SCSIFailure)
		return;
	// Stall the OUT endpoint if host data pending
	if (csw->dCSWDataResidue)
		epout_stall(usb, msc->ep_out, 1);

	// Send CSW after transfer finished
s_csw:	csw->bCSWStatus = ret.state == SCSIFailure;
	if (ret.state == SCSIGood || ret.state == SCSIFailure) {
		// Process new CBW packets
		msc->active = 0xff;
		while (msc->in_active != 0xff)
			__WFI();
		if (usb_ep_in_transfer(usb->base, msc->ep_in, csw, 13u) != 13u)
			dbgbkpt();
	}
}

usb_msc_t *usb_msc_init(usb_t *usb)
{
	usb_msc_t *data = &msc;
	memset(data, 0, sizeof(usb_msc_t));
	data->active = data->in_active = data->lun_max = 0xff;
	// Audio control interface
	const usb_if_t usbif = {
		.data = data,
		.config = &usbif_config,
		.setup_class = &usbif_setup_class,
	};
	usb_interface_register(usb, &usbif);
	return data;
}

scsi_t *usb_msc_scsi_register(usb_msc_t *msc, const scsi_if_t iface)
{
	// Allocate new space for SCSI object
	msc->lun_max++;
	msc->scsi = realloc(msc->scsi, sizeof(*msc->scsi) * (msc->lun_max + 1));

	// Initialise SCSI
	scsi_t *scsi = scsi_init(iface);
	msc->scsi[msc->lun_max] = scsi;

	// If failed
	if (!scsi)
		msc->lun_max--;
	return scsi;
}

void usb_msc_process(usb_t *usb, usb_msc_t *msc)
{
	if (!msc || msc->lun_max == 0xff)
		return;

	// Wait until FIFO available
	if (fifo_level(&msc->fifo) == MSC_FIFO_LEVEL)
		return;

	if (msc->active == 0xff)
		process_cmd(usb, msc);
	else
		process_data(usb, msc);
}
