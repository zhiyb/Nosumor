#include <malloc.h>
#include <string.h>
#include "../debug.h"
#include "usb_ep0.h"
#include "usb_ep.h"
#include "usb_ram.h"
#include "usb_macros.h"
#include "usb_structs.h"

#define MAX_SETUP_CNT	3u
#define MAX_PKT_CNT	1u
#define MAX_SIZE	64ul

#define DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ     (0 << 1)
#define DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ     (1 << 1)
#define DSTS_ENUMSPD_LS_PHY_6MHZ               (2 << 1)
#define DSTS_ENUMSPD_FS_PHY_48MHZ              (3 << 1)

typedef struct setup_buf_t {
	struct setup_buf_t * volatile next;
	// Setup packet buffers
	setup_t setup;
} setup_buf_t;

typedef struct {
	setup_buf_t * volatile pkt;
	uint8_t buf[MAX_SIZE] ALIGN(4);
} epout_data_t;

static void epin_init(usb_t *usb, uint32_t n)
{
	uint32_t size = MAX_SIZE * 2ul, addr = usb_ram_alloc(usb, &size);
	usb->base->DIEPTXF0_HNPTXFSIZ = DIEPTXF(addr, size);
	// Unmask interrupts
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
	ep->DIEPINT = 0;
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	dev->DAINTMSK |= DAINTMSK_IN(n);
}

void usb_ep0_enum(usb_t *usb, uint32_t speed)
{
	switch (speed) {
	case DSTS_ENUMSPD_LS_PHY_6MHZ:
		// LS: Maximum control packet size: 8 bytes
		usb->speed = USB_LowSpeed;
		dbgbkpt();
		break;
	case DSTS_ENUMSPD_FS_PHY_48MHZ:
	case DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ:
		usb->speed = USB_FullSpeed;
		// FS: Maximum control packet size: 64 bytes
		EP_IN(usb->base, 0)->DIEPCTL = 0;
		break;
	case DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ:
		usb->speed = USB_HighSpeed;
		// HS: Maximum control packet size: 64 bytes
		EP_IN(usb->base, 0)->DIEPCTL = 0;
	}
	// Allocate RX queue
	uint32_t size = usb_ram_size(usb) / 2;
	usb_ram_alloc(usb, &size);
	usb->base->GRXFSIZ = size / 4;
	// Initialise descriptors
	usb_desc_init(usb);
	// Initialise endpoint 0
	FUNC(usb->epin[0].init)(usb, 0);
	FUNC(usb->epout[0].init)(usb, 0);
}

uint32_t usb_ep0_max_size(USB_OTG_GlobalTypeDef *usb)
{
	static const uint32_t size[] = {64, 32, 16, 8};
	return size[FIELD(EP_OUT(usb, 0)->DOEPCTL, USB_OTG_DOEPCTL_MPSIZ)];
}

static inline void epout_reset(usb_t *usb, uint32_t n)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	epout_data_t *data = usb->epout[n].data;
	// Reset packet buffer
	ep->DOEPDMA = (uint32_t)&data->buf;
	// Reset packet counter
	ep->DOEPTSIZ = (MAX_SETUP_CNT << USB_OTG_DOEPTSIZ_STUPCNT_Pos) |
			(MAX_PKT_CNT << USB_OTG_DOEPTSIZ_PKTCNT_Pos) | MAX_SIZE;
	// Enable endpoint
	ep->DOEPCTL = USB_OTG_DOEPCTL_EPENA_Msk | USB_OTG_DOEPCTL_CNAK_Msk;
}

static void epout_init(usb_t *usb, uint32_t n)
{
	// Clear interrupts
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk |
			USB_OTG_DOEPINT_STUP_Msk | USB_OTG_DOEPINT_OTEPSPR_Msk;
	// Unmask interrupts
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	dev->DAINTMSK |= DAINTMSK_OUT(n);
	epout_reset(usb, n);
}

static void epout_pkt(usb_t *usb, uint32_t n)
{
	// Check data availability
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	uint32_t siz = ep->DOEPTSIZ;
	uint16_t setup_cnt = MAX_SETUP_CNT - FIELD(siz, USB_OTG_DOEPTSIZ_STUPCNT);
	uint16_t pkt_cnt = MAX_PKT_CNT - FIELD(siz, USB_OTG_DOEPTSIZ_PKTCNT);
	uint16_t size = MAX_SIZE - FIELD(siz, USB_OTG_DOEPTSIZ_XFRSIZ);
	// Receive data packets
	epout_data_t *data = usb->epout[n].data;
	void *dma = &data->buf;
	if (!size)
		return;
	if (pkt_cnt > MAX_PKT_CNT)
		dbgbkpt();
	if (setup_cnt > MAX_SETUP_CNT)
		dbgbkpt();
	if (setup_cnt)
		pkt_cnt = 0;
	// Find packets without data
	setup_buf_t * volatile *p;
	for (p = &data->pkt; *p; p = &(*p)->next) {
		// Iterate to end if no data available
		if (!pkt_cnt)
			continue;
		setup_t *setup = &(*p)->setup;
		uint16_t wLength = setup->wLength;
		if ((setup->bmRequestType & DIR_Msk) == DIR_H2D && wLength) {
			// Setup packet require a data OUT stage
			if (setup->data)
				continue;
			// Copy data packet
			void *buf = malloc(wLength);
			if (!buf || size < wLength)
				panic();
			memcpy(buf, dma, wLength);
			setup->data = buf;
			dma += wLength;
			size -= wLength;
			pkt_cnt--;
		}
	}
	// Append setup packets
	while (setup_cnt-- && size >= 8u) {
		// Copy setup packet
		setup_buf_t *setup = malloc(sizeof(setup_buf_t));
		if (!setup)
			panic();
		memcpy(&setup->setup, dma, 8u);
		*p = setup;
		dma += 8u;
		size -= 8u;
		setup->next = 0;
		setup->setup.data = 0;
		p = &setup->next;
	}
	if (size)
		dbgbkpt();
	epout_reset(usb, n);
}

void usb_ep0_register(usb_t *usb)
{
	const epin_t epin = {
		.data = 0,
		.init = &epin_init,
	};
	const epout_t epout = {
		.data = calloc(1u, sizeof(epout_data_t)),
		.init = &epout_init,
		.setup = &epout_pkt,
		.spr = &epout_pkt,
	};
	if (!epout.data)
		panic();
	int in, out;
	usb_ep_register(usb, &epin, &in, &epout, &out);
	if (in != 0 || out != 0)
		panic();
}

void usb_ep0_process(usb_t *usb)
{
	epout_data_t *data = usb->epout[0].data;
	if (!data->pkt)
		return;
	// Detach packets
	setup_buf_t * volatile *p = &data->pkt;
	while (*p) {
		setup_t *setup = &(*p)->setup;
		uint16_t wLength = setup->wLength;
		if ((setup->bmRequestType & DIR_Msk) == DIR_H2D && wLength)
			// Setup packet require a data OUT stage
			if (!setup->data) {
				// Skip if data not yet received
				p = &(*p)->next;
				continue;
			}
		// Process setup packet
		usb_setup(usb, 0, *setup);
		// Dequeue packet
		__disable_irq();
		setup_buf_t *t = *p;
		*p = (*p)->next;
		__enable_irq();
		free(t->setup.data);
		free(t);
	}
}
