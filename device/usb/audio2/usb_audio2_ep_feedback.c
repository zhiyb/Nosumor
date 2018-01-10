#include <malloc.h>
#include <system/systick.h>
#include "../usb.h"
#include "../usb_structs.h"
#include "../usb_macros.h"
#include "../usb_ep.h"
#include "../usb_irq.h"
#include "../usb_ram.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_ep_data.h"

// Frequency calculation update interval
#define INTERVAL	1024u
// Endpoint update repeats
#define EP_UPDATES	2u

uint32_t cnt = 0;

typedef struct {
	int32_t offset, acc;
	uint32_t freq, cnt;
	volatile int enabled, pending;
	// USB interface
	usb_t *usb;
	int ep;
} epdata_t;

static epdata_t epdata SECTION(.dtcm);

static void epin_init(usb_t *usb, uint32_t n)
{
	// Allocate TX FIFO
	uint32_t size = 4u, addr = usb_ram_alloc(usb, &size);
	usb->base->DIEPTXF[n - 1] = DIEPTXF(addr, size);
	// Clear interrupts
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
	ep->DIEPINT = USB_OTG_DIEPINT_XFRC_Msk;
	// Unmask interrupts
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	if (n == 1u) {
		dev->DEACHINT = USB_OTG_DEACHINT_IEP1INT_Msk;
		dev->DINEP1MSK = USB_OTG_DIEPINT_XFRC_Msk;
		dev->DEACHMSK |= USB_OTG_DEACHINTMSK_IEP1INTM_Msk;
	} else {
		dev->DAINTMSK |= DAINTMSK_IN(n);
	}
	// Configure endpoint
	ep->DIEPCTL = USB_OTG_DIEPCTL_USBAEP_Msk | EP_TYP_ISOCHRONOUS |
			(n << USB_OTG_DIEPCTL_TXFNUM_Pos) | (4u << USB_OTG_DIEPCTL_MPSIZ_Pos);
}

static void epin_update(usb_t *usb, uint32_t n)
{
	epdata_t *data = (epdata_t *)usb->epin[n].data;
	if (data->pending)
		return;
	// Check frame parity
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	uint32_t fn = FIELD(dev->DSTS, USB_OTG_DSTS_FNSOF) & 1u;
	fn = fn ? USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Msk : USB_OTG_DOEPCTL_SODDFRM_Msk;
	// Send frequency value
	USB_OTG_INEndpointTypeDef *ep = EP_IN(usb->base, n);
	ep->DIEPDMA = (uint32_t)&data->freq;
	ep->DIEPTSIZ = (1u << USB_OTG_DIEPTSIZ_PKTCNT_Pos) | 4u;
	// Enable endpoint
	ep->DIEPCTL |= fn | USB_OTG_DIEPCTL_EPENA_Msk | USB_OTG_DIEPCTL_CNAK_Msk;
	data->pending = 1;
	data->cnt--;
	cnt++;
}

static void epin_xfr_cplt(usb_t *usb, uint32_t n)
{
	epdata_t *data = (epdata_t *)usb->epin[n].data;
	// No more isochronous incomplete checks needed
	usb_isoc_check(usb, n | EP_DIR_IN, 0);
	// Enable endpoint
	data->pending = 0;
	if (!data->cnt)
		epin_update(usb, n);
}

static void feedback_update(uint32_t tick)
{
	epdata_t *data = &epdata;
	if (!data->enabled)
		return;
	// Update every INTERVAL timeouts
	if (tick & (INTERVAL - 1u))
		return;
	// Calculate feedback frequency
	int32_t offset = audio_buffering() / (int)AUDIO_FRAME_SIZE;
	int32_t diff = offset - data->offset;
	data->offset = offset;
	data->acc -= diff;
	// TODO: Variable frequency
	data->freq = (24ul << 16u) + data->acc;
	// Send update
	if (diff) {
		data->cnt = EP_UPDATES;
		epin_update(data->usb, data->ep);
		dbgprintf(ESC_MSG "[UAC2] Feedback: " ESC_DATA "%ld\n", data->acc);
	}
}

int usb_audio2_ep_feedback_register(usb_t *usb)
{
	memset(&epdata, 0, sizeof(epdata));
	epdata.usb = usb;
	const epin_t epin = {
		.data = &epdata,
		.init = &epin_init,
		.xfr_cplt = &epin_xfr_cplt,
	};
	usb_ep_register(usb, &epin, &epdata.ep, 0, 0);
	systick_register_handler(&feedback_update);
	return epdata.ep;
}

void usb_audio2_ep_feedback_halt(usb_t *usb, int n, int halt)
{
	epdata_t *data = (epdata_t *)usb->epin[n].data;
	if (!data)
		return;
	if (!halt) {
		// Reset statistics
		data->offset = 0;
		data->acc = 0;
		data->cnt = EP_UPDATES;
		// TODO: Variable frequency
		data->freq = 24ul << 16u;
		// Check for isochronous incomplete
		usb_isoc_check(epdata.usb, epdata.ep | EP_DIR_IN, 1);
		// Enable endpoint
		epin_update(usb, n);
	}
	data->enabled = !halt;
}

uint32_t usb_audio2_feedback_cnt()
{
	return cnt;
}
