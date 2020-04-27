#include <debug.h>
#include <device.h>
#include <system/systick.h>
#include <usb/usb.h>
#include <usb/usb_hw.h>
#include <usb/usb_macros.h>
#include "usb_ep0.h"
#include "usb_ep0_setup.h"

#define EP0_RX_SIZE	64
#define EP0_TX_SIZE	2048

typedef enum {BufFree = 0, BufActive, BufData, BufSetup, BufStatus} buf_state_t;

struct buf_t {
	void *p;
	volatile uint32_t size;
	uint32_t xfrsize;
	volatile buf_state_t state;
} brx[2], btx;

uint32_t brx_process;
volatile uint32_t brx_queue;

static void usb_ep0_out();
static void usb_ep0_irq_stup();
static void usb_ep0_irq_stsp();
static void usb_ep0_irq_xfrc(void *p, uint32_t size);

static void usb_ep0_enumeration(uint32_t spd)
{
	uint32_t size = 64;
	switch (spd) {
	case ENUM_FS:
	case ENUM_HS:
		size = 64;
		break;
#if DEBUG
	default:
		dbgprintf(ESC_WARNING "%lu\tusb_core: Unsupported enumeration speed: %lu\n",
			  systick_cnt(), spd);
		usb_connect(0);
		return;
#endif
	}

	// Try to allocate endpoint 0
	uint32_t epinnum = usb_hw_ep_alloc(UsbEp0, EP_DIR_IN, EP_CONTROL, size);
	uint32_t epoutnum = usb_hw_ep_alloc(UsbEp0, EP_DIR_OUT, EP_CONTROL, size);
#if DEBUG >= 5
	if (epinnum != 0 || epoutnum != 0)
		panic();
#endif

	// Allocate memories for endpoint 0
	brx[0].p = usb_hw_ram_alloc(EP0_RX_SIZE);
	brx[0].state = BufFree;
	brx[1].p = usb_hw_ram_alloc(EP0_RX_SIZE);
	brx[1].state = BufFree;
	brx_process = 0;
	brx_queue = 0;
	btx.p = usb_hw_ram_alloc(EP0_TX_SIZE);
	btx.state = BufFree;

	// Register interrupt handler
	usb_hw_ep_out_irq(epoutnum, &usb_ep0_irq_stup, &usb_ep0_irq_stsp, &usb_ep0_irq_xfrc);

	// Start OUT
	usb_ep0_out();
}

USB_ENUM_HANDLER(&usb_ep0_enumeration);

static void usb_ep0_irq_stup(void *p, uint32_t size)
{
	uint32_t rx = brx_queue;
	struct buf_t *buf = &brx[rx];
	buf->state = BufSetup;
	buf->size = size;
	brx_queue = rx = !rx;
	buf = &brx[rx];
	if (buf->state == BufFree)
		usb_ep0_out();
}

static void usb_ep0_irq_stsp(void *p, uint32_t size)
{
	USB_TODO();
	uint32_t rx = brx_queue;
	struct buf_t *buf = &brx[rx];
	buf->state = BufStatus;
	buf->size = size;
	brx_queue = rx = !rx;
	buf = &brx[rx];
	if (buf->state == BufFree)
		usb_ep0_out();
}

static void usb_ep0_irq_xfrc(void *p, uint32_t size)
{
}

static void usb_ep0_out()
{
	struct buf_t *buf = &brx[brx_queue];
#if DEBUG
	if (buf->state != BufFree)
		USB_ERROR();
#endif
	usb_hw_ep_out(0, buf->p, 1, 1, 0);
	buf->state = BufActive;
}

void *usb_ep0_in_buf(uint32_t *size)
{
	while (btx.state != BufFree)
		__WFI();
	if (size)
		*size = EP0_TX_SIZE;
	btx.state = BufData;
	return btx.p;
}

static void usb_ep0_in_irq(void *p, uint32_t size)
{
#if DEBUG
	if (btx.state != BufActive)
		USB_ERROR();
#endif
	btx.size += size;
	if (btx.size == btx.xfrsize && size != usb_hw_ep_max_size(EP_DIR_IN, 0)) {
		btx.state = BufFree;
		return;
	}
	USB_TODO();
	usb_hw_ep_in(0, btx.p + btx.size, btx.xfrsize - btx.size, usb_ep0_in_irq);
}

void usb_ep0_in(uint32_t size)
{
	if (btx.state != BufData)
		USB_ERROR();
	btx.state = BufActive;
	btx.xfrsize = size;
	btx.size = 0;

	uint32_t max_size = usb_hw_ep_max_size(EP_DIR_IN, 0);
	usb_hw_ep_in(0, btx.p, size >= max_size ? max_size : size, usb_ep0_in_irq);
}

void usb_ep0_in_stall()
{
#if DEBUG
	if (btx.state != BufFree && btx.state != BufData)
		USB_ERROR();
#endif
	USB_TODO();
}

static void usb_ep0_process()
{
	struct buf_t *buf = &brx[brx_process];
	buf_state_t state = buf->state;
	if (state == BufSetup || state == BufStatus) {
		if (state == BufSetup)
			usb_ep0_setup(buf->p, buf->size);
		else
			USB_TODO();
		buf->state = BufFree;
		brx_process = !brx_process;
		__disable_irq();
		state = brx[brx_queue].state;
		__enable_irq();
		// Restart OUT if it is currently NAK
		if (state == BufFree)
			usb_ep0_out();
	}
}

IDLE_HANDLER(&usb_ep0_process);
