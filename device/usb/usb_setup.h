#ifndef USB_SETUP_H
#define USB_SETUP_H

#include <stm32f7xx.h>

#define SETUP_TYPE_DIR_Pos	7u
#define SETUP_TYPE_DIR_Msk	(1ul << SETUP_TYPE_DIR_Pos)
#define SETUP_TYPE_DIR_H2D	(0ul << SETUP_TYPE_DIR_Pos)
#define SETUP_TYPE_DIR_D2H	(1ul << SETUP_TYPE_DIR_Pos)

#define SETUP_TYPE_TYPE_Pos	5u
#define SETUP_TYPE_TYPE_Msk	(3ul << SETUP_TYPE_TYPE_Pos)
#define SETUP_TYPE_TYPE_STD	(0ul << SETUP_TYPE_TYPE_Pos)
#define SETUP_TYPE_TYPE_CLASS	(1ul << SETUP_TYPE_TYPE_Pos)
#define SETUP_TYPE_TYPE_VENDOR	(2ul << SETUP_TYPE_TYPE_Pos)

#define SETUP_TYPE_RCPT_Pos	0u
#define SETUP_TYPE_RCPT_Msk	(0x1ful << SETUP_TYPE_RCPT_Pos)
#define SETUP_TYPE_RCPT_DEV	(0ul << SETUP_TYPE_RCPT_Pos)
#define SETUP_TYPE_RCPT_IF	(1ul << SETUP_TYPE_RCPT_Pos)
#define SETUP_TYPE_RCPT_EP	(2ul << SETUP_TYPE_RCPT_Pos)
#define SETUP_TYPE_RCPT_OTHER	(3ul << SETUP_TYPE_RCPT_Pos)

#define SETUP_REQ_GET_STATUS		0u
#define SETUP_REQ_CLEAR_FEATURE		1u
#define SETUP_REQ_SET_FEATURE		3u
#define SETUP_REQ_SET_ADDRESS		5u
#define SETUP_REQ_GET_DESCRIPTOR	6u
#define SETUP_REQ_SET_DESCRIPTOR	7u
#define SETUP_REQ_GET_CONFIGURATION	8u
#define SETUP_REQ_SET_CONFIGURATION	9u
#define SETUP_REQ_GET_INTERFACE		10u
#define SETUP_REQ_SET_INTERFACE		11u
#define SETUP_REQ_SYNCH_FRAME		12u

#define SETUP_DESC_TYPE_DEVICE		1u
#define SETUP_DESC_TYPE_CONFIGURATION	2u
#define SETUP_DESC_TYPE_STRING		3u
#define SETUP_DESC_TYPE_INTERFACE	4u
#define SETUP_DESC_TYPE_ENDPOINT	5u
#define SETUP_DESC_TYPE_DEVICE_QUALIFIER		6u
#define SETUP_DESC_TYPE_OTHER_SPEED_CONFIGURATION	7u
#define SETUP_DESC_TYPE_INTERACE_POWER	8u

typedef union setup_t {
	struct {
		uint8_t type;
		uint8_t request;
		union {
			uint16_t value;
			struct {
				uint8_t dindex;
				uint8_t dtype;
			};
		};
		uint16_t index;
		uint16_t length;
	};
	uint32_t raw[2];
} setup_t;

void usb_setup(USB_OTG_GlobalTypeDef *usb, uint32_t stat);

#endif // USB_SETUP_H
