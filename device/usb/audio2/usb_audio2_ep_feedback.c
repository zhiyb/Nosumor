#include <malloc.h>
#include "../usb.h"
#include "../usb_structs.h"
#include "../usb_macros.h"
#include "../usb_ep.h"
#include "../usb_irq.h"
#include "../usb_ram.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_ep_data.h"

uint32_t cnt = 0;

typedef struct {
	uint32_t cnt, freq;
	volatile int enabled, pending;
} epdata_t;

static void epin_init(usb_t *usb, uint32_t n)
{
	uint32_t size = 4u, addr = usb_ram_alloc(usb, &size);
	usb->base->DIEPTXF[n - 1] = DIEPTXF(addr, size);
	// Unmask interrupts
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
	ep->DIEPINT = USB_OTG_DIEPINT_XFRC_Msk;
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	dev->DAINTMSK |= DAINTMSK_IN(n);
	// Configure endpoint
	ep->DIEPCTL = USB_OTG_DIEPCTL_USBAEP_Msk | EP_TYP_ISOCHRONOUS |
			(n << USB_OTG_DIEPCTL_TXFNUM_Pos) | (4u << USB_OTG_DIEPCTL_MPSIZ_Pos);
}

static void epin_update(usb_t *usb, uint32_t n)
{
	epdata_t *data = (epdata_t *)usb->epin[n].data;
	if (data->pending)
		return;

	// Frame parity checking not needed
	// Frame parity will always be the same if interval != 1
#if 0
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	uint32_t fn = (FIELD(dev->DSTS, USB_OTG_DSTS_FNSOF)) & 1;
	fn = fn ? USB_OTG_DIEPCTL_SD0PID_SEVNFRM_Msk : USB_OTG_DIEPCTL_SODDFRM_Msk;
#endif

	// Calculate feedback frequency
	int16_t diff = -audio_buffering() + (AUDIO_FRAME_SIZE << 3u);
	// TODO: Variable frequency
	data->freq = (24ul << 16u) + (diff / 2);

	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
	ep->DIEPDMA = (uint32_t)&data->freq;
	ep->DIEPTSIZ = (1u << USB_OTG_DIEPTSIZ_PKTCNT_Pos) | 4u;
	// Enable endpoint
	DIEPCTL_SET(ep->DIEPCTL, USB_OTG_DIEPCTL_EPENA_Msk | USB_OTG_DIEPCTL_CNAK_Msk);

	data->pending = 1;
	cnt++;
}

static void epin_xfr_cplt(usb_t *usb, uint32_t n)
{
	epdata_t *data = (epdata_t *)usb->epin[n].data;
	data->pending = 0;
	// No more isochronous incomplete checks needed
	usb_isoc_check(usb, n | EP_DIR_IN, 0);
	epin_update(usb, n);
}

int usb_audio2_ep_feedback_register(usb_t *usb)
{
	epdata_t *epdata = calloc(1u, sizeof(epdata_t));
	if (!epdata)
		panic();

	const epin_t epin = {
		.data = epdata,
		.init = &epin_init,
		.xfr_cplt = &epin_xfr_cplt,
	};
	int ep;
	usb_ep_register(usb, &epin, &ep, 0, 0);
	return ep;
}

void usb_audio2_ep_feedback_halt(usb_t *usb, int n, int halt)
{
	epdata_t *data = (epdata_t *)usb->epin[n].data;
	if (!data)
		return;
	data->enabled = !halt;
	if (!halt) {
		usb_isoc_check(usb, n | EP_DIR_IN, 1);
		epin_update(usb, n);
	}
}

uint32_t usb_audio2_feedback_cnt()
{
	return cnt;
}
