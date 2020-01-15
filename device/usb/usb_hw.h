#ifndef USB_HW_H
#define USB_HW_H

#include <stdint.h>

uint32_t usb_hw_connected();
void usb_hw_connect(uint32_t e);

void *usb_hw_ram_alloc(uint32_t size);
uint32_t usb_hw_ep_alloc(uint32_t dir, uint32_t type, uint32_t size);

typedef void (*usb_ep_irq_stup_handler_t)(void *p, uint32_t size);	// Setup received
typedef void (*usb_ep_irq_stsp_handler_t)(void *p, uint32_t size);	// Status phase received
typedef void (*usb_ep_irq_xfrc_handler_t)(void *p, uint32_t size);	// Transfer complete

void usb_hw_ep_out_irq(uint32_t epnum, usb_ep_irq_stup_handler_t stup,
		       usb_ep_irq_stsp_handler_t stsp, usb_ep_irq_xfrc_handler_t xfrc);
void usb_hw_ep_out(uint32_t epnum, void *p, uint32_t setup, uint32_t pkt, uint32_t size);

#endif // USB_HW_H
