#include <stm32f7xx.h>
#include "../debug.h"
#include "usb_macros.h"
#include "usb_ep.h"
#include "usb_ep0.h"
#include "usb_ram.h"
#include "usb_structs.h"

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
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
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

void usb_isoc_check(usb_t *usb, uint8_t ep, int enable)
{
	uint32_t mask;
	if ((ep & EP_DIR_Msk) == EP_DIR_IN)
		mask = DAINTMSK_IN(ep & ~EP_DIR_Msk);
	else
		mask = DAINTMSK_OUT(ep & ~EP_DIR_Msk);
	if (enable)
		usb->episoc |= mask;
	else
		usb->episoc &= ~mask;
}

static inline void usb_endpoint_irq(usb_t *usb)
{
	USB_OTG_GlobalTypeDef *base = usb->base;
	USB_OTG_DeviceTypeDef *dev = DEV(base);
	uint32_t daint = dev->DAINT;
	uint32_t mask = FIELD(daint, USB_OTG_DAINT_OEPINT);
	for (uint32_t n = 0u; mask; n++, mask >>= 1u) {
		if (n == 1u || !(mask & 1u))
			continue;
		USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(base, n);
		uint32_t intr = ep->DOEPINT;
		if (intr & USB_OTG_DOEPINT_XFRC_Msk) {
			ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk;
			FUNC(usb->epout[n].xfr_cplt)(usb, n);
		}
		if (intr & (USB_OTG_DOEPINT_STUP_Msk | USB_OTG_DOEPINT_OTEPSPR_Msk)) {
			ep->DOEPINT = USB_OTG_DOEPINT_STUP_Msk | USB_OTG_DOEPINT_OTEPSPR_Msk;
			FUNC(usb->epout[n].setup_cplt)(usb, n);
		}
	}
	mask = FIELD(daint, USB_OTG_DAINT_IEPINT);
	for (uint32_t n = 0u; mask; n++, mask >>= 1u) {
		if (n == 1u || !(mask & 1u))
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
	USB_OTG_DeviceTypeDef *dev = DEV(base);
	uint32_t i = base->GINTSTS, msk = base->GINTMSK, bk = 1;
	uint32_t fn = FIELD(dev->DSTS, USB_OTG_DSTS_FNSOF) & 1ul;
	i &= msk;
	if (!i)
		return;
	if (i & (USB_OTG_GINTSTS_OEPINT_Msk | USB_OTG_GINTSTS_IEPINT_Msk)) {
		usb_endpoint_irq(usb);
		bk = 0;
	}
	if (i & USB_OTG_GINTSTS_IISOIXFR_Msk) {
		base->GINTSTS = USB_OTG_GINTSTS_IISOIXFR_Msk;
		uint32_t mask = FIELD(usb->episoc, USB_OTG_DAINT_IEPINT);
		if (!mask)
			base->GINTMSK &= ~USB_OTG_GINTMSK_IISOIXFRM_Msk;
		// Check frame parity
		uint32_t parity = fn ? USB_OTG_DIEPCTL_SD0PID_SEVNFRM_Msk : USB_OTG_DIEPCTL_SODDFRM_Msk;
		// Update endpoints
		for (uint32_t n = 1u; mask; n++, mask >>= 1u) {
			if (!(mask & 1u))
				continue;
			USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
			if (!(ep->DIEPCTL & USB_OTG_DIEPCTL_EPENA_Msk))
				continue;
			if ((ep->DIEPCTL & USB_OTG_DIEPCTL_EPTYP_Msk) != EP_TYP_ISOCHRONOUS)
				continue;
			if ((!(ep->DIEPCTL & USB_OTG_DIEPCTL_EONUM_DPID_Msk)) == fn)
				continue;
			ep->DIEPCTL |= parity;
			//putchar(fn + 'A');
		}
		bk = 0;
	}
	if (i & USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Msk) {
		base->GINTSTS = USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Msk;
		uint32_t mask = FIELD(usb->episoc, USB_OTG_DAINT_OEPINT);
		if (!mask)
			base->GINTMSK &= ~USB_OTG_GINTMSK_PXFRM_IISOOXFRM_Msk;
		// Check frame parity
		uint32_t parity = fn ? USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Msk : USB_OTG_DOEPCTL_SODDFRM_Msk;
		// Update endpoints
		for (uint32_t n = 1u; mask; n++, mask >>= 1u) {
			if (!(mask & 1u))
				continue;
			USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
			if (!(ep->DOEPCTL & USB_OTG_DOEPCTL_EPENA_Msk))
				continue;
			if ((ep->DOEPCTL & USB_OTG_DOEPCTL_EPTYP_Msk) != EP_TYP_ISOCHRONOUS)
				continue;
			// Not defined for DOEPCTL?
			if ((!(ep->DOEPCTL & USB_OTG_DIEPCTL_EONUM_DPID_Msk)) == fn)
				continue;
			ep->DOEPCTL |= parity;
			//putchar(fn + 'a');
		}
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
	if (bk)
		dbgbkpt();
	// Check isochronous incomplete interrupts
	if (FIELD(usb->episoc, USB_OTG_DAINT_IEPINT))
		msk |= USB_OTG_GINTMSK_IISOIXFRM_Msk;
	else
		msk &= ~USB_OTG_GINTMSK_IISOIXFRM_Msk;
	if (FIELD(usb->episoc, USB_OTG_DAINT_OEPINT))
		msk |= USB_OTG_GINTMSK_PXFRM_IISOOXFRM_Msk;
	else
		msk &= ~USB_OTG_GINTMSK_PXFRM_IISOOXFRM_Msk;
	// Update interrupt masks
	if (msk != base->GINTMSK)
		base->GINTMSK = msk;
}

void OTG_HS_EP1_IN_IRQHandler()
{
	usb_t *usb = usb_hs;
	USB_OTG_GlobalTypeDef *base = usb->base;
	USB_OTG_DeviceTypeDef *dev = DEV(base);
	dev->DEACHINT = USB_OTG_DEACHINT_IEP1INT_Msk;
	USB_OTG_INEndpointTypeDef *ep = EP_IN(base, 1);
	uint32_t intr = ep->DIEPINT;
	if (intr & USB_OTG_DIEPINT_XFRC_Msk) {
		ep->DIEPINT = USB_OTG_DIEPINT_XFRC_Msk;
		FUNC(usb->epin[1].xfr_cplt)(usb, 1);
	}
	if (intr & USB_OTG_DIEPINT_TOC_Msk) {
		ep->DIEPINT = USB_OTG_DIEPINT_TOC_Msk;
		FUNC(usb->epin[1].timeout)(usb, 1);
	}
}

void OTG_HS_EP1_OUT_IRQHandler()
{
	usb_t *usb = usb_hs;
	USB_OTG_GlobalTypeDef *base = usb->base;
	USB_OTG_DeviceTypeDef *dev = DEV(base);
	dev->DEACHINT = USB_OTG_DEACHINT_OEP1INT_Msk;
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(base, 1);
	uint32_t intr = ep->DOEPINT;
	if (intr & USB_OTG_DOEPINT_XFRC_Msk) {
		ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk;
		FUNC(usb->epout[1].xfr_cplt)(usb, 1);
	}
	if (intr & (USB_OTG_DOEPINT_STUP_Msk | USB_OTG_DOEPINT_OTEPSPR_Msk)) {
		ep->DOEPINT = USB_OTG_DOEPINT_STUP_Msk | USB_OTG_DOEPINT_OTEPSPR_Msk;
		FUNC(usb->epout[1].setup_cplt)(usb, 1);
	}
}
