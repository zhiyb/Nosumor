#include <debug.h>
#include <device.h>
#include <system/systick.h>
#include <usb/usb.h>
#include <usb/usb_hw.h>
#include <usb/usb_macros.h>
#include "usb_ep0.h"

#define EP0_RX_SIZE	64
#define EP0_TX_SIZE	2048

typedef enum {BUF_FREE = 0, BUF_VALID} buf_state_t;

struct buf_t {
	void *p;
	volatile buf_state_t state;
} brx[2], btx;

volatile uint32_t brx_process, brx_queue;

static void usb_ep0_out();

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
	uint32_t epinnum = usb_hw_ep_alloc(EP_DIR_IN, EP_CONTROL, size);
	uint32_t epoutnum = usb_hw_ep_alloc(EP_DIR_OUT, EP_CONTROL, size);
#if DEBUG
	if (epinnum != 0 || epoutnum != 0)
		panic();
#endif

	// Allocate memories for endpoint 0
	brx[0].p = usb_hw_ram_alloc(EP0_RX_SIZE);
	brx[0].state = BUF_FREE;
	brx[1].p = usb_hw_ram_alloc(EP0_RX_SIZE);
	brx[1].state = BUF_FREE;
	brx_process = 0;
	brx_queue = 0;
	btx.p = usb_hw_ram_alloc(EP0_TX_SIZE);
	btx.state = BUF_FREE;

	// Start Rx
	usb_ep0_out();
}

USB_ENUM_HANDLER(&usb_ep0_enumeration);

static void usb_ep0_out()
{
	struct buf_t *buf = &brx[brx_queue];
	if (buf->state != BUF_FREE)
		return;
	usb_hw_ep_out(0, buf->p, 1, 1, 0);
	brx_queue = !brx_queue;
}

static void usb_ep0_process()
{
	if (brx[brx_process].state == BUF_VALID) {
		USB_TODO();
	}
}

IDLE_HANDLER(&usb_ep0_process);
