#include <debug.h>
#include "usb_ep0.h"
#include "usb_ep.h"
#include "usb_ram.h"
#include "usb_macros.h"
#include "usb_setup.h"

#define DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ     (0 << 1)
#define DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ     (1 << 1)
#define DSTS_ENUMSPD_LS_PHY_6MHZ               (2 << 1)
#define DSTS_ENUMSPD_FS_PHY_48MHZ              (3 << 1)

static void epin_init(usb_t *usb)
{
	uint32_t size = 128, addr = usb_ram_alloc(usb, &size);
	usb->base->DIEPTXF0_HNPTXFSIZ = DIEPTXF(addr, size);
}

void usb_ep0_enum(USB_OTG_GlobalTypeDef *usb, uint32_t speed)
{
	switch (speed) {
	case DSTS_ENUMSPD_LS_PHY_6MHZ:
		// LS: Maximum control packet size: 8 bytes
		dbgbkpt();
		break;
	case DSTS_ENUMSPD_FS_PHY_48MHZ:
	case DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ:
		// FS: Maximum control packet size: 64 bytes
	case DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ:
		// HS: Maximum control packet size: 64 bytes
		EP_IN(usb, 0)->DIEPCTL = 0;
		EP_OUT(usb, 0)->DOEPTSIZ = USB_OTG_DOEPTSIZ_STUPCNT_Msk |
				USB_OTG_DOEPTSIZ_PKTCNT_Msk | 64;
		break;
	}
	// Enable endpoint 0 OUT
	EP_OUT(usb, 0)->DOEPCTL = USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;
}

uint32_t usb_ep0_max_size(USB_OTG_GlobalTypeDef *usb)
{
	static const uint32_t size[] = {64, 32, 16, 8};
	return size[(EP_OUT(usb, 0)->DOEPCTL & USB_OTG_DOEPCTL_MPSIZ_Msk)
			>> USB_OTG_DOEPCTL_MPSIZ_Pos];
}

void epout_recv(usb_t *usb, uint32_t stat)
{
	switch (stat & USB_OTG_GRXSTSP_PKTSTS) {
	case STAT_OUT_NAK:
		dbgbkpt();
		break;
	case STAT_OUT_RECV:
		if (STAT_CNT(stat) != 0)
			dbgbkpt();
		break;
	case STAT_OUT:
		if (STAT_CNT(stat) != 0)
			dbgbkpt();
		break;
	case STAT_SETUP:
		if (STAT_CNT(stat) != 0)
			dbgbkpt();
		break;
	case STAT_SETUP_RECV:
		usb_setup(usb, stat);
		break;
	default:
		dbgbkpt();
	}
}

void usb_ep0_register(usb_t *usb)
{
	const epin_t epin = {
		epin_init,
	};
	const epout_t epout = {
		0, epout_recv,
	};
	int in, out;
	usb_ep_register(usb, &epin, &in, &epout, &out);
	if (in != 0 || out != 0)
		dbgbkpt();
}
