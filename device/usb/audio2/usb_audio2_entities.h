#ifndef USB_AUDIO2_ENTITIES_H
#define USB_AUDIO2_ENTITIES_H

#include "../usb_setup.h"

typedef struct data_t data_t;

void usb_audio2_entities_init(data_t *data);

void usb_audio2_get(usb_t *usb, data_t *data, uint32_t ep, setup_t pkt);
void usb_audio2_set(usb_t *usb, data_t *data, uint32_t ep, setup_t pkt);

#endif // USB_AUDIO2_ENTITIES_H
