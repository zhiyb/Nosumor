#include <stdio.h>
#include <stm32f7xx.h>
#include <debug.h>
#include <escape.h>
#include "usb_macros.h"

void OTG_HS_WKUP_IRQHandler()
{
	USB_OTG_GlobalTypeDef *usb = USB_OTG_HS;
	dbbkpt();
}

void OTG_HS_IRQHandler()
{
	USB_OTG_GlobalTypeDef *usb = USB_OTG_HS;
	USB_OTG_DeviceTypeDef *dev = DEVICE(usb);
	uint32_t i = usb->GINTSTS, bk = 1;
	if (i & USB_OTG_GINTSTS_OTGINT) {
		dbbkpt();
	}
	if (i & USB_OTG_GINTSTS_MMIS) {
		dbbkpt();
	}
	if (i & USB_OTG_GINTSTS_RXFLVL) {
		dbbkpt();
	}
	if (i & USB_OTG_GINTSTS_USBRST) {
		usb->GINTSTS = USB_OTG_GINTSTS_USBRST;
		bk = 0;
	}
	if (i & USB_OTG_GINTSTS_ENUMDNE) {
		switch (dev->DSTS & USB_OTG_DSTS_ENUMSPD) {
		case DSTS_ENUMSPD_LS_PHY_6MHZ:
		case DSTS_ENUMSPD_FS_PHY_48MHZ:
		case DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ:
		case DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ:
			dbbkpt();
			break;
		}
		INEP(usb, 0)->DIEPCTL = USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_STALL;
	}
	if (bk)
		dbbkpt();
}

void OTG_HS_EP1_IN_IRQHandler()
{
	USB_OTG_GlobalTypeDef *usb = USB_OTG_HS;
	dbbkpt();
}

void OTG_HS_EP1_OUT_IRQHandler()
{
	USB_OTG_GlobalTypeDef *usb = USB_OTG_HS;
	dbbkpt();
}
