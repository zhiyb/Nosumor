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

static struct {
	struct buf_t {
		void *p;
		volatile uint32_t size;
		uint32_t xfrsize;
		volatile buf_state_t state;
	} brx[2], btx;
	volatile uint32_t brx_queue;
	uint32_t brx_process;
} data;

static void usb_ep0_out();
static void usb_ep0_out_irq_stup();
static void usb_ep0_out_irq_stsp();

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
	data.brx[0].p = usb_hw_ram_alloc(EP0_RX_SIZE);
	data.brx[0].state = BufFree;
	data.brx[1].p = usb_hw_ram_alloc(EP0_RX_SIZE);
	data.brx[1].state = BufFree;
	data.brx_process = 0;
	data.brx_queue = 0;
	data.btx.p = usb_hw_ram_alloc(EP0_TX_SIZE);
	data.btx.state = BufFree;

	// Register interrupt handler
	usb_hw_ep_out_irq(epoutnum, &usb_ep0_out_irq_stup, &usb_ep0_out_irq_stsp, 0);

	// Start OUT
	usb_ep0_out();
}

USB_ENUM_HANDLER(&usb_ep0_enumeration);

static void usb_ep0_out_irq_stup(void *p, uint32_t size)
{
	uint32_t rx = data.brx_queue;
	struct buf_t *buf = &data.brx[rx];
#if DEBUG >= 5
	if (buf->state != BufActive)
		USB_ERROR();
#endif
#if DEBUG >= 6
	printf(ESC_DEBUG "%lu\tusb_ep0: " ESC_READ "EP 0 OUT"
	       ESC_DEBUG " received %p setup %lu btyes\n",
	       systick_cnt(), buf, size);
#endif
	buf->state = BufSetup;
	buf->size = size;
	data.brx_queue = rx = !rx;
	buf = &data.brx[rx];
	if (buf->state == BufFree)
		usb_ep0_out();
}

static void usb_ep0_out_irq_stsp(void *p, uint32_t size)
{
	uint32_t rx = data.brx_queue;
	struct buf_t *buf = &data.brx[rx];
#if DEBUG >= 5
	if (buf->state != BufActive)
		USB_ERROR();
#endif
#if DEBUG >= 6
	printf(ESC_DEBUG "%lu\tusb_ep0: " ESC_READ "EP 0 OUT"
	       ESC_DEBUG " received %p status %lu btyes\n",
	       systick_cnt(), buf, size);
#endif
	buf->state = BufStatus;
	buf->size = size;
	data.brx_queue = rx = !rx;
	buf = &data.brx[rx];
	if (buf->state == BufFree)
		usb_ep0_out();
}

static void usb_ep0_out()
{
	struct buf_t *buf = &data.brx[data.brx_queue];
#if DEBUG
	if (buf->state != BufFree)
		USB_ERROR();
#endif
	usb_hw_ep_out(0, buf->p, 1, 1, 0);
	buf->state = BufActive;
}

void *usb_ep0_in_buf(uint32_t *size)
{
	while (data.btx.state != BufFree)
		__WFI();
	if (size)
		*size = EP0_TX_SIZE;
	data.btx.state = BufData;
	return data.btx.p;
}

static void usb_ep0_in_irq(void *p, uint32_t size)
{
#if DEBUG
	if (data.btx.state != BufActive)
		USB_ERROR();
#endif
#if DEBUG >= 6
	printf(ESC_DEBUG "%lu\tusb_ep0: " ESC_WRITE "EP 0 IN" ESC_DEBUG " irq transferred %lu bytes\n",
	       systick_cnt(), size);
#endif
	data.btx.size += size;
	if (data.btx.size == data.btx.xfrsize && size != usb_hw_ep_max_size(EP_DIR_IN, 0)) {
		data.btx.state = BufFree;
		return;
	}

	uint32_t max_size = usb_hw_ep_max_size(EP_DIR_IN, 0);
	size = data.btx.xfrsize - data.btx.size;
	size = size >= max_size ? max_size : size;
	usb_hw_ep_in(0, data.btx.p + data.btx.size, size, usb_ep0_in_irq);
}

void usb_ep0_in(uint32_t size)
{
	if (data.btx.state != BufData)
		USB_ERROR();
	data.btx.state = BufActive;
	data.btx.xfrsize = size;
	data.btx.size = 0;

	uint32_t max_size = usb_hw_ep_max_size(EP_DIR_IN, 0);
	usb_hw_ep_in(0, data.btx.p, size >= max_size ? max_size : size, usb_ep0_in_irq);
}

void usb_ep0_in_stall()
{
#if DEBUG >= 5
	if (data.btx.state != BufFree && data.btx.state != BufData)
		USB_ERROR();
#endif
	USB_TODO();
}

void usb_ep0_in_free()
{
#if DEBUG >= 5
	if (data.btx.state != BufData)
		USB_ERROR();
#endif
	data.btx.state = BufFree;
}

void usb_ep0_setup(const void *p, uint32_t size);
void usb_ep0_status(const void *p, uint32_t size);

static void usb_ep0_process()
{
	struct buf_t *buf = &data.brx[data.brx_process];
	buf_state_t state = buf->state;
	if (state == BufSetup || state == BufStatus) {
#if DEBUG >= 6
		printf(ESC_DEBUG "%lu\tusb_ep0: " ESC_READ "EP 0 OUT"
		       ESC_DEBUG " process packet %p type %u\n",
		       systick_cnt(), buf, state);
#endif
		if (state == BufSetup)
			usb_ep0_setup(buf->p, buf->size);
		else
			usb_ep0_status(buf->p, buf->size);
		__disable_irq();
		buf->state = BufFree;
		data.brx_process = !data.brx_process;
		state = data.brx[data.brx_queue].state;
		__enable_irq();
		// Restart OUT if it is currently NAK
		if (state == BufFree)
			usb_ep0_out();
	}
}

IDLE_HANDLER(&usb_ep0_process);
