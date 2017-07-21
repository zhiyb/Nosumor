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
#define MAX_SIZE	64ul

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
	return size[(EP_OUT(usb, 0)->DOEPCTL & USB_OTG_DOEPCTL_MPSIZ_Msk)
			>> USB_OTG_DOEPCTL_MPSIZ_Pos];
}

static void epout_init(usb_t *usb, uint32_t n)
{
	void *data = usb->epout[n].data ?: malloc(MAX_SIZE);
	usb->epout[n].data = data;
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	// Configure endpoint DMA
	ep->DOEPDMA = (uint32_t)data;
	// Reset packet counter
	ep->DOEPTSIZ = (MAX_SETUP_CNT << USB_OTG_DOEPTSIZ_STUPCNT_Pos) |
			(MAX_PKT_CNT << USB_OTG_DOEPTSIZ_PKTCNT_Pos) | MAX_SIZE;
	// Enable endpoint
	ep->DOEPCTL = USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;
	// Clear interrupts
	ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk | USB_OTG_DOEPINT_STUP_Msk;
	// Unmask interrupts
	USB_OTG_DeviceTypeDef *dev = DEVICE(usb->base);
	dev->DAINTMSK |= DAINTMSK_OUT(n);
}

static void epout_xfr_cplt(usb_t *usb, uint32_t n)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	uint32_t siz = ep->DOEPTSIZ;
	//uint32_t setup_cnt = MAX_SETUP_CNT - FIELD(siz, USB_OTG_DOEPTSIZ_STUPCNT);
	//uint32_t pkt_cnt = MAX_PKT_CNT - FIELD(siz, USB_OTG_DOEPTSIZ_PKTCNT);
	uint32_t size = MAX_SIZE - FIELD(siz, USB_OTG_DOEPTSIZ_XFRSIZ);
	if (size == 0u)
		return;
	if (size & 7u)
		dbgbkpt();
	uint32_t setup_cnt = size >> 3u;
	// Receive packets
	setup_t *setup = (setup_t *)usb->epout[0].data;
	while (setup_cnt--)
		usb_setup(usb, 0, *setup++);
}

static void epout_setup_cplt(usb_t *usb, uint32_t n)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	// Update device address
	if (usb->addr)
		DEVICE(usb->base)->DCFG = (DEVICE(usb->base)->DCFG & ~USB_OTG_DCFG_DAD_Msk) |
				((usb->addr << USB_OTG_DCFG_DAD_Pos) & USB_OTG_DCFG_DAD_Msk);
	// Configure endpoint DMA
	ep->DOEPDMA = (uint32_t)usb->epout[n].data;
	// Reset packet counter
	ep->DOEPTSIZ = (MAX_SETUP_CNT << USB_OTG_DOEPTSIZ_STUPCNT_Pos) |
			(MAX_PKT_CNT << USB_OTG_DOEPTSIZ_PKTCNT_Pos) | MAX_SIZE;
	// Enable endpoint OUT
	ep->DOEPCTL = USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;
}

void usb_ep0_register(usb_t *usb)
{
	static const epin_t epin = {
		.data = 0,
		.init = epin_init,
	};
	static const epout_t epout = {
		.data = 0,
		.init = epout_init,
		.xfr_cplt = epout_xfr_cplt,
		.setup_cplt = epout_setup_cplt,
	};
	int in, out;
	usb_ep_register(usb, &epin, &in, &epout, &out);
	if (in != 0 || out != 0)
		dbgbkpt();
}
