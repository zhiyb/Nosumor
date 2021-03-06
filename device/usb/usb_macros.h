#ifndef USB_MACROS_H
#define USB_MACROS_H

#include <stdint.h>
#include <list.h>

// Common USB static lists
typedef void (*const usb_basic_handler_t)();
#define USB_INIT_HANDLER(func)		LIST_ITEM(usb_init, usb_basic_handler_t) = func
#define USB_SUSP_HANDLER(func)		LIST_ITEM(usb_susp, usb_basic_handler_t) = func
#define USB_RESET_HANDLER(func)		LIST_ITEM(usb_reset, usb_basic_handler_t) = func
typedef void (*const usb_enum_handler_t)(uint32_t spd);
#define USB_ENUM_HANDLER(func)		LIST_ITEM(usb_enum, usb_enum_handler_t) = func
typedef void (*const usb_config_handler_t)(uint8_t config);
#define USB_CONFIG_HANDLER(func)	LIST_ITEM(usb_config, usb_config_handler_t) = func

// Enumeration speed
#define ENUM_HS		0b00
#define ENUM_FS		0b01

// Endpoint direction
#define EP_DIR_Msk	0x80
#define EP_DIR_IN	0x80
#define EP_DIR_OUT	0x00

// Transfer type
#define EP_CONTROL	0x00
#define EP_ISOCHRONOUS	0x01
#define EP_BULK		0x02
#define EP_INTERRUPT	0x03

// Iso mode synchronisation type
#define EP_ISO_NONE	0x00
#define EP_ISO_ASYNC	0x04
#define EP_ISO_ADAPTIVE	0x08
#define EP_ISO_SYNC	0x0c

// Iso mode usage type
#define EP_ISO_DATA	0x00
#define EP_ISO_FEEDBACK	0x10
#define EP_ISO_IMPLICIT	0x20

// Language IDs
#define LANG_ZH_CN	0x0804
#define LANG_EN_US	0x0409
#define LANG_EN_GB	0x0809

#if DEBUG
#include <debug.h>
#include <system/systick.h>
#include <usb/usb.h>

#define USB_TODO()	do { \
		dbgprintf(ESC_WARNING "%lu\tusb_core: TODO @ " ESC_DATA "%s:%u: %s()\n", \
			systick_cnt(), __FILE__, __LINE__, __PRETTY_FUNCTION__); \
		usb_connect(0); \
		dbgbkpt(); \
	} while (0)
#define USB_ERROR()	do { \
		dbgprintf(ESC_ERROR "%lu\tusb_core: ERROR @ " ESC_DATA "%s:%u: %s()\n", \
			systick_cnt(), __FILE__, __LINE__, __PRETTY_FUNCTION__); \
		usb_connect(0); \
		dbgbkpt(); \
	} while (0)
#else
#define USB_TODO()	((void)0)
#define USB_ERROR()	do { \
		dbgprintf(ESC_ERROR "%lu\tusb_core: ERROR @ " ESC_DATA "%s:%u\n", \
			systick_cnt(), __FILE__, __LINE__); \
		usb_connect(0); \
	} while (0)
#endif

#endif // USB_MACROS_H
