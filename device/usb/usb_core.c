#include <debug.h>
#include <macros.h>
#include <list.h>
#include <system/systick.h>
#include "usb_macros.h"
#include "usb_hw.h"
#include "usb_core.h"

void usb_init()
{
}

uint32_t usb_connected()
{
	return usb_hw_connected();
}

void usb_connect(uint32_t e)
{
	usb_hw_connect(e);
	dbgprintf(ESC_MSG "%lu\tusb_core: USB %sconnected\n", systick_cnt(),
		usb_connected() ? ESC_ENABLE : ESC_DISABLE "dis");
}

void usb_core_reset()
{
}

void usb_core_enumeration(uint32_t spd)
{
	if (spd != ENUM_HS) {
		dbgprintf(ESC_WARNING "%lu\tusb_core: Unsupported enumeration speed: %lu\n",
			  systick_cnt(), spd);
		usb_connect(0);
		return;
	}
	USB_TODO();
}
