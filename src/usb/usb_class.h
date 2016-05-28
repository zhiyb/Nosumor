#ifndef USB_CLASS_H
#define USB_CLASS_H

#include <stdint.h>

union setup_value_class_t {
	uint8_t id;
	uint8_t value;
};

#include "usb_def.h"

void usbClassSetup(struct setup_t *setup);
void usbClassSetupInterface(struct setup_t *setup);

#endif // USB_CLASS_H
