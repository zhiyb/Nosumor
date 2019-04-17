#include <stm32f7xx.h>
#include <device.h>
#include <debug.h>
#include "usb_macros.h"
#include "usb_ep.h"

static void usb_reset(USB_OTG_GlobalTypeDef *base)
{
	USB_OTG_DeviceTypeDef *dev = DEV(base);
	// Reset USB device address
	dev->DCFG &= ~USB_OTG_DCFG_DAD_Msk;
	dev->DAINTMSK = 0;
}

#if 0
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
#endif

#if 0
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
#endif

static inline void usb_endpoint_in_irq(USB_OTG_GlobalTypeDef *base)
{
	USB_OTG_DeviceTypeDef *dev = DEV(base);
	uint32_t daint = dev->DAINT & dev->DAINTMSK;
	uint32_t mask = FIELD(daint, USB_OTG_DAINT_IEPINT);
	for (uint32_t n = 0u; mask; n++, mask >>= 1u) {
		if (!(mask & 1u))
			continue;
		USB_OTG_INEndpointTypeDef *ep = EP_IN(base, n);
		uint32_t intr = ep->DIEPINT;

		// Transfer complete
		if (intr & USB_OTG_DIEPINT_XFRC_Msk) {
			ep->DIEPINT = USB_OTG_DIEPINT_XFRC_Msk;
			//FUNC(usb->epin[n].xfr_cplt)(usb, n);
			dbgbkpt();
		}

		// Endpoint timeout
		if (intr & USB_OTG_DIEPINT_TOC_Msk) {
			ep->DIEPINT = USB_OTG_DIEPINT_TOC_Msk;
			//FUNC(usb->epin[n].timeout)(usb, n);
			dbgbkpt();
		}
	}
}

static inline void usb_endpoint_out_irq(USB_OTG_GlobalTypeDef *base)
{
	USB_OTG_DeviceTypeDef *dev = DEV(base);
	uint32_t daint = dev->DAINT & dev->DAINTMSK;
	uint32_t mask = FIELD(daint, USB_OTG_DAINT_OEPINT);
	for (uint32_t n = 0u; mask; n++, mask >>= 1u) {
		if (!(mask & 1u))
			continue;
		USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(base, n);
		uint32_t intr = ep->DOEPINT;

		// Transfer complete
		if (intr & USB_OTG_DOEPINT_XFRC_Msk) {
			ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk;
			//FUNC(usb->epout[n].xfr_cplt)(usb, n);
			dbgbkpt();
		}

		// Setup packet received
		if (intr & (USB_OTG_DOEPINT_STUP_Msk)) {
			ep->DOEPINT = USB_OTG_DOEPINT_STUP_Msk;
			//FUNC(usb->epout[n].setup)(usb, n);
			dbgbkpt();
		}

		// Status phase received
		if (intr & (USB_OTG_DOEPINT_OTEPSPR_Msk)) {
			ep->DOEPINT = USB_OTG_DOEPINT_OTEPSPR_Msk;
			//FUNC(usb->epout[n].spr)(usb, n);
			dbgbkpt();
		}
	}
}

void OTG_HS_WKUP_IRQHandler()
{
	USB_OTG_GlobalTypeDef *base = USB_OTG_HS;
	UNUSED(base);
	dbgbkpt();
}

void OTG_HS_IRQHandler()
{
	//usb_t *usb = usb_hs;
	//USB_OTG_GlobalTypeDef *base = usb->base;
	USB_OTG_GlobalTypeDef *base = USB_OTG_HS;
	USB_OTG_DeviceTypeDef *dev = DEV(base);
	uint32_t i = base->GINTSTS, bkpt = 1;
	// Frame parity for isochronous transfers
	uint32_t fn = FIELD(dev->DSTS, USB_OTG_DSTS_FNSOF) & 1ul;
	i &= base->GINTMSK;
	// Return if no interrupt to process
	if (!i)
		return;
	//usb->active++;

#ifndef BOOTLOADER
	// Missed IN isochronous transfer frame
	if (i & USB_OTG_GINTSTS_IISOIXFR_Msk) {
		base->GINTSTS = USB_OTG_GINTSTS_IISOIXFR_Msk;
#if 0
		// Disable interrupt if no IN isochronous endpoints enabled
		uint32_t mask = FIELD(usb->episoc, USB_OTG_DAINT_IEPINT);
		if (!mask)
			base->GINTMSK &= ~USB_OTG_GINTMSK_IISOIXFRM_Msk;
#endif

		dbgbkpt();
#if 0
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
		}
#endif
		bkpt = 0;
	}

	// Missed OUT isochronous transfer frame
	if (i & USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Msk) {
		base->GINTSTS = USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Msk;
#if 0
		// Disable interrupt if no OUT isochronous endpoints enabled
		uint32_t mask = FIELD(usb->episoc, USB_OTG_DAINT_OEPINT);
		if (!mask)
			base->GINTMSK &= ~USB_OTG_GINTMSK_PXFRM_IISOOXFRM_Msk;
#endif

		dbgbkpt();
#if 0
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
		}
#endif
		bkpt = 0;
	}
#endif

	// IN endpoint events
	if (i & (USB_OTG_GINTSTS_OEPINT_Msk | USB_OTG_GINTSTS_IEPINT_Msk)) {
		usb_endpoint_in_irq(base);
		bkpt = 0;
	}

	// OUT endpoint events
	if (i & (USB_OTG_GINTSTS_OEPINT_Msk | USB_OTG_GINTSTS_IEPINT_Msk)) {
		usb_endpoint_out_irq(base);
		bkpt = 0;
	}

	// OTG event
	if (i & USB_OTG_GINTSTS_OTGINT_Msk) {
		// Ignore it
		bkpt = 0;
	}

	// USB mode mismatch event
	if (i & USB_OTG_GINTSTS_MMIS_Msk) {
		dbgbkpt();
		base->GINTSTS = USB_OTG_GINTSTS_MMIS_Msk;
		bkpt = 0;
	}

	// USB suspend
	if (i & (USB_OTG_GINTSTS_USBSUSP_Msk)) {
		base->GINTSTS = USB_OTG_GINTSTS_USBSUSP_Msk;
		bkpt = 0;
	}

	// USB reset
	if (i & USB_OTG_GINTSTS_USBRST_Msk) {
		usb_reset(base);
		base->GINTSTS = USB_OTG_GINTSTS_USBRST_Msk;
		bkpt = 0;
	}

	// USB emulation done
	if (i & USB_OTG_GINTSTS_ENUMDNE_Msk) {
		//usb_ep0_enum(usb, dev->DSTS & USB_OTG_DSTS_ENUMSPD_Msk);
		base->GINTSTS = USB_OTG_GINTSTS_ENUMDNE_Msk;
		bkpt = 0;
	}

	// Unimplemented interrupt
	if (bkpt)
		dbgbkpt();
}

void OTG_HS_EP1_IN_IRQHandler()
{
#if 0
	usb_t *usb = usb_hs;
	USB_OTG_GlobalTypeDef *base = usb->base;
	USB_OTG_DeviceTypeDef *dev = DEV(base);
	USB_OTG_INEndpointTypeDef *ep = EP_IN(base, 1);
	uint32_t intr = ep->DIEPINT & dev->DINEP1MSK;
	if (intr & USB_OTG_DIEPINT_XFRC_Msk) {
		FUNC(usb->epin[1].xfr_cplt)(usb, 1);
		ep->DIEPINT = USB_OTG_DIEPINT_XFRC_Msk;
	}
	if (intr & USB_OTG_DIEPINT_TOC_Msk) {
		FUNC(usb->epin[1].timeout)(usb, 1);
		ep->DIEPINT = USB_OTG_DIEPINT_TOC_Msk;
	}
#else
	dbgbkpt();
#endif
}

void OTG_HS_EP1_OUT_IRQHandler()
{
#if 0
	usb_t *usb = usb_hs;
	USB_OTG_GlobalTypeDef *base = usb->base;
	USB_OTG_DeviceTypeDef *dev = DEV(base);
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(base, 1);
	uint32_t intr = ep->DOEPINT & dev->DOUTEP1MSK;
	if (intr & USB_OTG_DOEPINT_XFRC_Msk) {
		ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk;
		FUNC(usb->epout[1].xfr_cplt)(usb, 1);
	}
	if (intr & USB_OTG_DOEPINT_STUP_Msk) {
		ep->DOEPINT = USB_OTG_DOEPINT_STUP_Msk;
		FUNC(usb->epout[1].setup)(usb, 1);
	}
	if (intr & USB_OTG_DOEPINT_OTEPSPR_Msk) {
		ep->DOEPINT = USB_OTG_DOEPINT_OTEPSPR_Msk;
		FUNC(usb->epout[1].setup)(usb, 1);
	}
#else
	dbgbkpt();
#endif
}
