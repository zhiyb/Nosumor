#ifndef USB_DESC_H
#define USB_DESC_H

#include <stdint.h>
#include <stm32f7xx.h>

typedef struct desc_t {
	uint32_t size;
	const char *p;
} desc_t;

desc_t usb_desc_device(USB_OTG_GlobalTypeDef *usb);

#endif // USB_DESC_H
