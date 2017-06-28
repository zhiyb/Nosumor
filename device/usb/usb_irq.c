#include <stdio.h>
#include <stm32f7xx.h>
#include <debug.h>
#include <escape.h>
#include "usb_macros.h"
#include "usb_ep.h"
#include "usb_ep0.h"
#include "usb_ram.h"
#include "usb_setup.h"
#include "usb.h"

usb_t *usb_hs = 0;

void usb_hs_irq_init(usb_t *usb)
{
	usb_hs = usb;
	// Interrupt masks
	usb->base->GINTSTS = 0xffffffff;
	usb->base->GINTMSK = USB_OTG_GINTMSK_OTGINT | USB_OTG_GINTMSK_MMISM;
}

static void usb_reset(usb_t *usb)
{
	USB_OTG_DeviceTypeDef *dev = DEVICE(usb);
	// Reset USB device address
	dev->DCFG &= ~USB_OTG_DCFG_DAD_Msk;
	// Set endpoint 0 NAK
	EP_OUT(usb, 0)->DOEPCTL = USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_SNAK;
	// Disable other endpoints
	for (int i = 1; i != USB_EPOUT_CNT; i++)
		EP_OUT(usb, i)->DOEPCTL = USB_OTG_DOEPCTL_EPDIS;
	// Reset USB FIFO RAM
	usb_ram_reset(usb);
	// Allocate RX queue
	uint32_t size = usb_ram_size(usb) / 2;
	usb_ram_alloc(usb, &size);
	usb->base->GRXFSIZ = size / 4;
	FUNC(usb->epin[0].init)(usb);
	FUNC(usb->epout[0].init)(usb);
	// Unmask interrupts
	dev->DAINTMSK = DAINTMSK_IN(0) | DAINTMSK_OUT(0);
}

void OTG_HS_WKUP_IRQHandler()
{
	USB_OTG_GlobalTypeDef *usb = usb_hs->base;
	dbgbkpt();
}

void OTG_HS_IRQHandler()
{
	USB_OTG_GlobalTypeDef *usb = usb_hs->base;
	USB_OTG_DeviceTypeDef *dev = DEVICE(usb);
	uint32_t i = usb->GINTSTS, bk = 1;
	if (i & USB_OTG_GINTSTS_OTGINT) {
		dbgbkpt();
	}
	if (i & USB_OTG_GINTSTS_MMIS) {
		dbgbkpt();
	}
	if (i & USB_OTG_GINTSTS_RXFLVL) {
		usb->GINTMSK &= ~USB_OTG_GINTMSK_RXFLVLM_Msk;
		uint32_t stat = usb->GRXSTSP;
		FUNC(usb_hs->epout[STAT_EP(stat)].recv)(usb_hs, stat);
		bk = 0;
		usb->GINTMSK |= USB_OTG_GINTMSK_RXFLVLM_Msk;
	}
	if (i & USB_OTG_GINTSTS_USBRST) {
		usb_reset(usb_hs);
		usb->GINTSTS = USB_OTG_GINTSTS_USBRST;
		bk = 0;
	}
	if (i & USB_OTG_GINTSTS_ENUMDNE) {
		usb_ep0_enum(usb, dev->DSTS & USB_OTG_DSTS_ENUMSPD);
		usb->GINTSTS = USB_OTG_GINTSTS_ENUMDNE;
		bk = 0;
	}
	if (bk)
		dbgbkpt();
}

void OTG_HS_EP1_IN_IRQHandler()
{
	USB_OTG_GlobalTypeDef *usb = usb_hs->base;
	dbgbkpt();
}

void OTG_HS_EP1_OUT_IRQHandler()
{
	USB_OTG_GlobalTypeDef *usb = usb_hs->base;
	dbgbkpt();
}
