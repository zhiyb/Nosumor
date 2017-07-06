#include <stdio.h>
#include <stm32f7xx.h>
#include "usb_debug.h"
#include "usb_macros.h"
#include "usb_ep.h"
#include "usb_ep0.h"
#include "usb_ram.h"
#include "usb_setup.h"
#include "usb_desc.h"
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
	USB_OTG_DeviceTypeDef *dev = DEVICE(usb->base);
	// Reset USB device address
	dev->DCFG &= ~USB_OTG_DCFG_DAD_Msk;
	dev->DAINTMSK = 0;
	// Reset USB FIFO RAM
	usb_ram_reset(usb);
	// Allocate RX queue
	uint32_t size = usb_ram_size(usb) / 2;
	usb_ram_alloc(usb, &size);
	usb->base->GRXFSIZ = size / 4;
	FUNC(usb->epin[0].init)(usb, 0);
	FUNC(usb->epout[0].init)(usb, 0);
	usb_desc_init(usb);
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
	if (i & USB_OTG_GINTSTS_OEPINT_Msk) {
		for (uint32_t n = 0u; n != USB_EPOUT_CNT; n++) {
			USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb, n);
			if (ep->DOEPINT & USB_OTG_DOEPINT_XFRC_Msk) {
				FUNC(usb_hs->epout[n].xfr_cplt)(usb_hs, n);
				ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk;
			}
			if (ep->DOEPINT & USB_OTG_DOEPINT_STUP_Msk) {
				FUNC(usb_hs->epout[n].setup_cplt)(usb_hs, n);
				ep->DOEPINT = USB_OTG_DOEPINT_STUP_Msk;
			}
		}
		bk = 0;
	}
	if (i & USB_OTG_GINTSTS_IEPINT_Msk) {
		for (uint32_t n = 0u; n != USB_EPIN_CNT; n++) {
			USB_OTG_INEndpointTypeDef *ep = EP_IN(usb, n);
			if (ep->DIEPINT & USB_OTG_DIEPINT_XFRC_Msk) {
				FUNC(usb_hs->epin[n].xfr_cplt)(usb_hs, n);
				ep->DIEPINT = USB_OTG_DIEPINT_XFRC_Msk;
			}
			if (ep->DIEPINT & USB_OTG_DIEPINT_TOC_Msk) {
				FUNC(usb_hs->epin[n].timeout)(usb_hs, n);
				ep->DIEPINT = USB_OTG_DIEPINT_TOC_Msk;
			}
		}
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
