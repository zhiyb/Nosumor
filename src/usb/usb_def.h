#ifndef USB_DEF_H
#define USB_DEF_H

#include <stdint.h>
#include "stm32f1xx.h"
#include "usb.h"

#define USB_EPnR(id)	(*(&USB->EP0R + (id) * (&USB->EP1R - &USB->EP0R)))

struct setup_value_descriptor_t {
	uint8_t index;
	uint8_t type;
};

struct setup_t {
	uint8_t type, request;
	uint16_t RESERVED0;
	union {
		uint16_t value;
		struct setup_value_descriptor_t descriptor;
	};
	uint16_t RESERVED1;
	uint16_t index;
	uint16_t RESERVED2;
	uint16_t length;
	uint16_t RESERVED3;
};

struct status_t {
	struct {
		volatile uint8_t active;
		uint8_t epid;
		uint8_t dir;
		uint8_t size;
		uint8_t buffSize;
		const void *ptr;
	} dma;
	uint8_t addr, config;
};

extern struct status_t usbStatus;

#endif // USB_DEF_H
