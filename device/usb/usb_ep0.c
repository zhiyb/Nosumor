#include <malloc.h>
#include "usb_debug.h"
#include "usb_ep0.h"
#include "usb_ep.h"
#include "usb_ram.h"
#include "usb_macros.h"
#include "usb_setup.h"
#include "usb_desc.h"

#define MAX_SETUP_CNT	3u
#define MAX_PKT_CNT	1u
#define MAX_SIZE	32ul

#define DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ     (0 << 1)
#define DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ     (1 << 1)
#define DSTS_ENUMSPD_LS_PHY_6MHZ               (2 << 1)
#define DSTS_ENUMSPD_FS_PHY_48MHZ              (3 << 1)

static void epin_init(usb_t *usb, uint32_t n)
{
	uint32_t size = 256, addr = usb_ram_alloc(usb, &size);
	usb->base->DIEPTXF0_HNPTXFSIZ = DIEPTXF(addr, size);
	// Unmask interrupts
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
	ep->DIEPINT = 0;
	USB_OTG_DeviceTypeDef *dev = DEVICE(usb->base);
	dev->DAINTMSK |= DAINTMSK_IN(n);
}

void usb_ep0_enum(usb_t *usb, uint32_t speed)
{
	switch (speed) {
	case DSTS_ENUMSPD_LS_PHY_6MHZ:
		// LS: Maximum control packet size: 8 bytes
		usb->speed = USB_LowSpeed;
		dbgbkpt();
		break;
	case DSTS_ENUMSPD_FS_PHY_48MHZ:
	case DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ:
		usb->speed = USB_FullSpeed;
		// FS: Maximum control packet size: 64 bytes
		EP_IN(usb->base, 0)->DIEPCTL = 0;
		break;
	case DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ:
		usb->speed = USB_HighSpeed;
		// HS: Maximum control packet size: 64 bytes
		EP_IN(usb->base, 0)->DIEPCTL = 0;
	}
	// Initialise descriptors and endpoints
	usb_desc_init(usb);
	for (int i = 0; i != USB_EPIN_CNT; i++)
		FUNC(usb->epin[i].init)(usb, i);
	for (int i = 0; i != USB_EPIN_CNT; i++)
		FUNC(usb->epout[i].init)(usb, i);
}

uint32_t usb_ep0_max_size(USB_OTG_GlobalTypeDef *usb)
{
	static const uint32_t size[] = {64, 32, 16, 8};
	return size[FIELD(EP_OUT(usb, 0)->DOEPCTL, USB_OTG_DOEPCTL_MPSIZ)];
}

static void epout_init(usb_t *usb, uint32_t n)
{
	void *data = usb->epout[n].data ?: malloc(MAX_SIZE);
	usb->epout[n].data = data;
	// Clear interrupts
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk | USB_OTG_DOEPINT_STUP_Msk;
	// Unmask interrupts
	USB_OTG_DeviceTypeDef *dev = DEVICE(usb->base);
	dev->DAINTMSK |= DAINTMSK_OUT(n);
	// Receive setup packets
	usb_ep_out_transfer(usb->base, n, data,
			    MAX_SETUP_CNT, MAX_PKT_CNT, MAX_SIZE);
}

static void epout_xfr_cplt(usb_t *usb, uint32_t n)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	uint32_t siz = ep->DOEPTSIZ;
	uint16_t setup_cnt = MAX_SETUP_CNT - FIELD(siz, USB_OTG_DOEPTSIZ_STUPCNT);
	uint16_t pkt_cnt = MAX_PKT_CNT - FIELD(siz, USB_OTG_DOEPTSIZ_PKTCNT);
	uint16_t size = MAX_SIZE - FIELD(siz, USB_OTG_DOEPTSIZ_XFRSIZ);
	if (setup_cnt) {
		// Copy setup packet
		memcpy(&usb->setup, usb->epout[n].data, 8u);
	} else if (pkt_cnt && size) {
		// Copy data packet
		usb->setup_buf = realloc(usb->setup_buf, size);
		memcpy(usb->setup_buf, usb->epout[n].data, size);
		// Reset packet buffer
		ep->DOEPDMA = (uint32_t)usb->epout[n].data;
		// Enable endpoint
		ep->DOEPCTL = USB_OTG_DOEPCTL_EPENA_Msk | USB_OTG_DOEPCTL_CNAK_Msk;
		// Process setup with data
		usb_setup(usb, n, usb->setup);
		// Clear mystery interrupt flags
		// Otherwise setup interrupts will not be triggered
		ep->DOEPINT = 0x2030;
	}
}

static void epout_setup_cplt(usb_t *usb, uint32_t n)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	uint32_t siz = ep->DOEPTSIZ;
	// Reset packet counter
	ep->DOEPTSIZ = (MAX_SETUP_CNT << USB_OTG_DOEPTSIZ_STUPCNT_Pos) |
			(MAX_PKT_CNT << USB_OTG_DOEPTSIZ_PKTCNT_Pos) | MAX_SIZE;
	// Reset packet buffer
	ep->DOEPDMA = (uint32_t)usb->epout[n].data;
	// Enable endpoint
	ep->DOEPCTL = USB_OTG_DOEPCTL_EPENA_Msk | USB_OTG_DOEPCTL_CNAK_Msk;
	// Process setup packet
	uint16_t setup_cnt = MAX_SETUP_CNT - FIELD(siz, USB_OTG_DOEPTSIZ_STUPCNT);
	if (!setup_cnt) {
		dbgbkpt();
		return;
	}
	if ((usb->setup.bmRequestType & SETUP_TYPE_DIR_Msk) == SETUP_TYPE_DIR_H2D &&
			usb->setup.wLength)
		// Setup packet require a data OUT stage
		return;
	usb_setup(usb, n, usb->setup);
}

void usb_ep0_register(usb_t *usb)
{
	static const epin_t epin = {
		.data = 0,
		.init = &epin_init,
	};
	static const epout_t epout = {
		.data = 0,
		.init = &epout_init,
		.xfr_cplt = &epout_xfr_cplt,
		.setup_cplt = &epout_setup_cplt,
	};
	int in, out;
	usb_ep_register(usb, &epin, &in, &epout, &out);
	if (in != 0 || out != 0)
		dbgbkpt();
}
