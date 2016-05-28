#ifndef USB_CLASS_H
#define USB_CLASS_H

#include <stdint.h>

#include "usb_def.h"

#define EP1_SIZE	8

void usbClassInit();
void usbClassReset();
void usbClassHalt(uint16_t epaddr, uint16_t e);
void usbClassSetup(struct setup_t *setup);
void usbClassSetupInterface(struct setup_t *setup);

#endif // USB_CLASS_H
