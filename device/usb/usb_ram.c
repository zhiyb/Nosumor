#include <stm32f7xx.h>
#include "usb_ram.h"
#include "usb_macros.h"

uint32_t usb_ram_alloc(usb_t *usb, uint32_t *size)
{
	*size &= ~3ul;	// Align to 32-bit words
	if (usb->top + *size >= usb_ram_size(usb))
		*size = usb->max - usb->top - 1;
	uint32_t addr = usb->top;
	usb->top += *size;
	return addr;
}

void usb_ram_reset(usb_t *usb)
{
	usb->top = 0;
	usb->fifo = 0;
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
