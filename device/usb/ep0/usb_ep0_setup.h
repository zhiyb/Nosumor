#ifndef USB_EP0_SETUP_H
#define USB_EP0_SETUP_H

#include <stdint.h>
#include <macros.h>

#define USB_SETUP_TYPE_DIR_Pos		7u
#define USB_SETUP_TYPE_DIR_Msk		(1ul << USB_SETUP_TYPE_DIR_Pos)
#define USB_SETUP_TYPE_DIR_H2D		(0ul << USB_SETUP_TYPE_DIR_Pos)
#define USB_SETUP_TYPE_DIR_D2H		(1ul << USB_SETUP_TYPE_DIR_Pos)

#define USB_SETUP_TYPE_TYPE_Pos		5u
#define USB_SETUP_TYPE_TYPE_Msk		(3ul << USB_SETUP_TYPE_TYPE_Pos)
#define USB_SETUP_TYPE_TYPE_STD		(0ul << USB_SETUP_TYPE_TYPE_Pos)
#define USB_SETUP_TYPE_TYPE_CLASS	(1ul << USB_SETUP_TYPE_TYPE_Pos)
#define USB_SETUP_TYPE_TYPE_VENDOR	(2ul << USB_SETUP_TYPE_TYPE_Pos)

#define USB_SETUP_GET_STATUS		0u
#define USB_SETUP_CLEAR_FEATURE		1u
#define USB_SETUP_SET_FEATURE		3u
#define USB_SETUP_SET_ADDRESS		5u
#define USB_SETUP_GET_DESCRIPTOR	6u
#define USB_SETUP_SET_DESCRIPTOR	7u
#define USB_SETUP_GET_CONFIGURATION	8u
#define USB_SETUP_SET_CONFIGURATION	9u
#define USB_SETUP_GET_INTERFACE		10u
#define USB_SETUP_SET_INTERFACE		11u
#define USB_SETUP_SYNCH_FRAME		12u

#define USB_SETUP_FEATURE_ENDPOINT_HALT		0u
#define USB_SETUP_FEATURE_DEVICE_REMOTE_WAKEUP	1u
#define USB_SETUP_FEATURE_TEST_MODE		2u

typedef union PACKED {
	struct PACKED {
		uint8_t bmRequestType;
		uint8_t bRequest;
		union PACKED {
			uint16_t wValue;
			struct PACKED {
				uint8_t bIndex;
				uint8_t bType;
			};
		};
		union PACKED {
			uint16_t wIndex;
			struct PACKED {
				uint8_t bID;
				uint8_t bEntityID;
			};
		};
		uint16_t wLength;
	};
	uint32_t raw[2];
} usb_setup_t;

#include <usb/usb_desc.h>

typedef void (*usb_ep0_iface_setup_t)(usb_setup_t pkt, usb_desc_t *pdesc);
void usb_ep0_iface_register(uint8_t iface, usb_ep0_iface_setup_t fsetup);

typedef uint32_t (*usb_ep0_status_t)(usb_setup_t pkt, const void *p, uint32_t size);
void usb_ep0_status_register(usb_setup_t pkt, usb_ep0_status_t fstatus);

#endif // USB_EP0_SETUP_H
