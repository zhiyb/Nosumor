#ifndef USB_SETUP_H
#define USB_SETUP_H

#include <stm32f7xx.h>
#include "../macros.h"

#define DIR_Pos	7u
#define DIR_Msk	(1ul << DIR_Pos)
#define DIR_H2D	(0ul << DIR_Pos)
#define DIR_D2H	(1ul << DIR_Pos)

#define GET_STATUS		0u
#define CLEAR_FEATURE		1u
#define SET_FEATURE		3u
#define SET_ADDRESS		5u
#define GET_DESCRIPTOR		6u
#define SET_DESCRIPTOR		7u
#define GET_CONFIGURATION	8u
#define SET_CONFIGURATION	9u
#define GET_INTERFACE		10u
#define SET_INTERFACE		11u
#define SYNCH_FRAME		12u

#define FEATURE_ENDPOINT_HALT		0u
#define FEATURE_DEVICE_REMOTE_WAKEUP	1u
#define FEATURE_TEST_MODE		2u

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usb_t usb_t;

// Setup packet
typedef struct setup_t {
	union PACKED {
		struct {
			uint8_t bmRequestType;
			uint8_t bRequest;
			union {
				uint16_t wValue;
				struct {
					uint8_t bIndex;
					uint8_t bType;
				};
			};
			union {
				uint16_t wIndex;
				struct {
					uint8_t bID;
					uint8_t bEntityID;
				};
			};
			uint16_t wLength;
		};
		uint32_t raw[2];
	};
	void *data ALIGNED;
} setup_t;

// Standard setup handler
void usb_setup(usb_t *usb, uint32_t n, setup_t pkt);

#ifdef __cplusplus
}
#endif

#endif // USB_SETUP_H
