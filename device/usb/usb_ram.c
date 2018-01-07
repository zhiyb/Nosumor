#include <malloc.h>
#include "../debug.h"
#include "usb_ram.h"
#include "usb_structs.h"

uint32_t usb_ram_alloc(usb_t *usb, uint32_t *size)
{
	*size = (*size + 3ul) & ~3ul;	// Align to 32-bit words
	if (usb->top + *size >= usb_ram_size(usb)) {
		dbgbkpt();
		*size = usb->max - usb->top - 1;
	}
	uint32_t addr = usb->top;
	usb->top += *size;
	return addr >> 2u;
}

void usb_ram_reset(usb_t *usb)
{
	usb->top = 0;
	usb->fifo = 0;
}

uint32_t usb_ram_usage(usb_t *usb)
{
	return usb->top;
}

uint32_t usb_ram_size(usb_t *usb)
{
	if (usb->max == 0)
		usb->max = usb->base->GRXFSIZ << 2u;
	return usb->max;
}

uint32_t usb_ram_fifo_alloc(usb_t *usb)
{
	return ++usb->fifo;
}

void usb_interface_register(usb_t *usb, const usb_if_t *usbif)
{
	usb_if_t **p;
	for (p = &usb->usbif; *p != 0; p = &(*p)->next);
	*p = (usb_if_t *)malloc(sizeof(usb_if_t));
	if (!p)
		panic();
	**p = *usbif;
	(*p)->next = 0;
}
