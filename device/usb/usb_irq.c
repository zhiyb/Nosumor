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
	if (usb->base == USB_OTG_HS)
		usb_hs = usb;
	else
		dbgbkpt();
}

static void usb_reset(usb_t *usb)
{
	USB_OTG_DeviceTypeDef *dev = DEVICE(usb->base);
	// Reset USB device address
	dev->DCFG &= ~USB_OTG_DCFG_DAD_Msk;
	dev->DAINTMSK = 0;
	// Reset USB FIFO RAM allocation
	usb_ram_reset(usb);
	// Allocate RX queue
	uint32_t size = usb_ram_size(usb) / 2;
	usb_ram_alloc(usb, &size);
	usb->base->GRXFSIZ = size / 4;
	usb->speed = USB_Reset;
}

void usb_disable(usb_t *usb)
{
	// Disable endpoint 0
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, 0);
	if (ep->DIEPCTL & USB_OTG_DIEPCTL_EPENA_Msk) {
		DIEPCTL_SET(ep->DIEPCTL, USB_OTG_DIEPCTL_EPDIS_Msk);
		while (ep->DIEPCTL & USB_OTG_DIEPCTL_EPENA_Msk);
	}
	// Disable other endpoints
	for (usb_if_t **pi = &usb->usbif; *pi != 0; pi = &(*pi)->next)
		FUNC((*pi)->disable)(usb, (*pi)->data);
	// Flush FIFOs
	usb->base->GRSTCTL = (0b10000ul << USB_OTG_GRSTCTL_FCRST_Pos) |
			USB_OTG_GRSTCTL_TXFFLSH_Msk | USB_OTG_GRSTCTL_RXFFLSH_Msk;
	usb->speed = USB_Reset;
}

static inline void usb_endpoint_irq(usb_t *usb)
{
	USB_OTG_GlobalTypeDef *base = usb->base;
	USB_OTG_DeviceTypeDef *dev = DEVICE(base);
	uint32_t daint = dev->DAINT;
	uint32_t mask = daint >> 16;
	for (uint32_t n = 0u; n != USB_EPOUT_CNT; n++, mask >>= 1u) {
		if (!(mask & 1u))
			continue;
		USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(base, n);
		uint32_t intr = ep->DOEPINT;
		if (intr & USB_OTG_DOEPINT_XFRC_Msk) {
			ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk;
			FUNC(usb->epout[n].xfr_cplt)(usb, n);
		}
		if (intr & USB_OTG_DOEPINT_STUP_Msk) {
			ep->DOEPINT = USB_OTG_DOEPINT_STUP_Msk;
			FUNC(usb->epout[n].setup_cplt)(usb, n);
		}
	}
	mask = daint;
	for (uint32_t n = 0u; n != USB_EPIN_CNT; n++, mask >>= 1u) {
		if (!(mask & 1u))
			continue;
		USB_OTG_INEndpointTypeDef *ep = EP_IN(base, n);
		uint32_t intr = ep->DIEPINT;
		if (intr & USB_OTG_DIEPINT_XFRC_Msk) {
			ep->DIEPINT = USB_OTG_DIEPINT_XFRC_Msk;
			FUNC(usb->epin[n].xfr_cplt)(usb, n);
		}
		if (intr & USB_OTG_DIEPINT_TOC_Msk) {
			ep->DIEPINT = USB_OTG_DIEPINT_TOC_Msk;
			FUNC(usb->epin[n].timeout)(usb, n);
		}
	}
}

void OTG_HS_WKUP_IRQHandler()
{
	USB_OTG_GlobalTypeDef *usb = usb_hs->base;
	dbgbkpt();
}

void OTG_HS_IRQHandler()
{
	usb_t *usb = usb_hs;
	USB_OTG_GlobalTypeDef *base = usb->base;
	USB_OTG_DeviceTypeDef *dev = DEVICE(base);
	uint32_t i = base->GINTSTS, bk = 1, fn = FIELD(dev->DSTS, USB_OTG_DSTS_FNSOF);
	i &= base->GINTMSK;
	if (!i)
		return;
	if (i & (USB_OTG_GINTSTS_OEPINT_Msk | USB_OTG_GINTSTS_IEPINT_Msk)) {
		usb_endpoint_irq(usb);
		bk = 0;
	}
	if (i & USB_OTG_GINTSTS_OTGINT) {
		dbgbkpt();
	}
	if (i & USB_OTG_GINTSTS_MMIS) {
		dbgbkpt();
	}
	if (i & (USB_OTG_GINTSTS_USBSUSP_Msk)) {
		base->GINTSTS = USB_OTG_GINTSTS_USBSUSP_Msk;
		bk = 0;
	}
	if (i & USB_OTG_GINTSTS_USBRST_Msk) {
		usb_reset(usb);
		base->GINTSTS = USB_OTG_GINTSTS_USBRST_Msk;
		bk = 0;
	}
	if (i & USB_OTG_GINTSTS_ENUMDNE_Msk) {
		usb_ep0_enum(usb, dev->DSTS & USB_OTG_DSTS_ENUMSPD_Msk);
		base->GINTSTS = USB_OTG_GINTSTS_ENUMDNE_Msk;
		bk = 0;
	}
	if (i & USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Msk) {
		//dbgbkpt();
		base->GINTSTS = USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Msk;
		putchar((fn & 1) + '0');
		fflush(stdout);
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
