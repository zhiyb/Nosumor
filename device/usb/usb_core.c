#include <debug.h>
#include <macros.h>
#include <list.h>
#include <system/systick.h>
#include "usb_macros.h"
#include "usb_hw.h"
#include "usb_core.h"

LIST(usb_init, usb_basic_handler_t);
LIST(usb_reset, usb_basic_handler_t);
LIST(usb_enum, usb_enum_handler_t);

void usb_init()
{
	LIST_ITERATE(usb_init, usb_basic_handler_t, p) (*p)();
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
	LIST_ITERATE(usb_reset, usb_basic_handler_t, p) (*p)();
}

void usb_core_enumeration(uint32_t spd)
{
#if DEBUG >= 5
	if (LIST_SIZE(usb_enum) == 0)
		USB_TODO();
#endif
	LIST_ITERATE(usb_enum, usb_enum_handler_t, p) (*p)(spd);
}
