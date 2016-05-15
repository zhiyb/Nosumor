#include <stdint.h>
#include "stm32f1xx.h"
#include "keyboard.h"
#include "macros.h"
#include "usb.h"
#include "usb_def.h"
#include "debug.h"

extern uint32_t _susbram;

struct ep_t eptable[8][2] __attribute__((section(".usbtable")));

__IO uint32_t ep0rx[MAX_EP0_SIZE / 2] __attribute__((section(".usbram")));
uint32_t ep0tx[MAX_EP0_SIZE / 2] __attribute__((section(".usbram")));

struct status_t usbStatus;

void initUSB()
{
	usbStatus.dma.active = 0;

	// Enable clock and NVIC
	RCC->APB1ENR |= RCC_APB1ENR_USBEN;
	uint32_t prioritygroup = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, NVIC_EncodePriority(prioritygroup, 1, 0));
	NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, NVIC_EncodePriority(prioritygroup, 2, 0));
	NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
	NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

	// Enable USB peripheral
	USB->CNTR = USB_CNTR_FRES;
	// At least 1 us
	for (volatile uint32_t i = 0; i != 72; i++);
	USB->CNTR = 0;
	// Buffer allocation table address
	USB->BTABLE = USB_LOCAL_ADDR(eptable);

	// Clear interrupt flags
	USB->ISTR = 0;
	// Enable interrupt masks
	USB->CNTR = USB_CNTR_CTRM | USB_CNTR_PMAOVRM | /*USB_CNTR_ERRM |*/
			USB_CNTR_WKUPM | USB_CNTR_SUSPM | USB_CNTR_RESETM /*|
			USB_CNTR_SOFM | USB_CNTR_ESOFM*/;
	eptable[0][EP_TX].addr = USB_LOCAL_ADDR(ep0tx);
	eptable[0][EP_TX].count = 0;
	eptable[0][EP_RX].addr = USB_LOCAL_ADDR(&ep0rx);
	eptable[0][EP_RX].count = USB_RX_COUNT(sizeof(ep0rx) / 2);
}

static void usbReset()
{
	usbStatus.addr = 0;
	// Configure endpoint 0
	USB->EP0R = USB_EP_CONTROL | USB_EP_RX_VALID | USB_EP_TX_VALID;
	// Start USB device
	USB->DADDR = USB_DADDR_EF;
}

static void usbCTR()
{
	uint32_t epid = USB->ISTR & USB_ISTR_EP_ID;
	uint32_t dir = USB->ISTR & USB_ISTR_DIR ? 1 : 0;
	struct ep_t *ep = &eptable[epid][dir];

	volatile uint16_t *epr = &USB->EP0R + epid;
	uint32_t eprv = *epr;
	uint32_t epmasked = (eprv & (USB_EP_TYPE_MASK | USB_EP_KIND | USB_EPADDR_FIELD))
			| USB_EP_CTR_RX | USB_EP_CTR_TX;

	uint32_t bkpt = 1;
	if (eprv & USB_EP_CTR_RX) {
		if (eprv & USB_EP_SETUP) {
			usbSetup(epid);
			bkpt = 0;
		}
		if ((ep->count & USB_RX_COUNT_MASK) == 0)
			bkpt = 0;
		if (bkpt)
			dbbkpt();
		bkpt = 0;
		*epr = (epmasked & ~USB_EP_CTR_RX) | (USB_EP0R_STAT_RX ^ (eprv & USB_EP0R_STAT_RX));
	}
	if (eprv & USB_EP_CTR_TX) {
		if (usbStatus.addr) {
			USB->DADDR = usbStatus.addr | USB_DADDR_EF;
			usbStatus.addr = 0;
		}
		bkpt = 0;
		*epr = epmasked & ~USB_EP_CTR_TX;
	}
	if (bkpt)
		dbbkpt();
	USB->ISTR = (uint16_t)~USB_ISTR_CTR;
}

void USB_HP_CAN1_TX_IRQHandler()
{
	USB_TypeDef *usb = USB;
	toggleLED(LED_LEFT);
	dbputc('H');
	dbbkpt();
}

void USB_LP_CAN1_RX0_IRQHandler()
{
	toggleLED(LED_RIGHT);
	uint16_t istr;

	// Correct transfer
	while ((istr = USB->ISTR) & USB_ISTR_CTR)
		usbCTR();

	if (istr & USB_ISTR_PMAOVR) {
		// Packet memory overrun / underrun
		dbbkpt();
		USB->ISTR = (uint16_t)~USB_ISTR_PMAOVR;
	}
	if (istr & USB_ISTR_ERR) {
		// Error
		USB->ISTR = (uint16_t)~USB_ISTR_ERR;
	}
	if (istr & USB_ISTR_WKUP) {
		// Wakeup request
		USB->ISTR = (uint16_t)~USB_ISTR_WKUP;
		USB->CNTR &= ~USB_CNTR_FSUSP;
	}
	if (istr & USB_ISTR_SUSP) {
		// Suspend request
		USB->ISTR = (uint16_t)~USB_ISTR_SUSP;
		USB->CNTR |= USB_CNTR_FSUSP;
		USB->CNTR |= USB_CNTR_LP_MODE;
	}
	if (istr & USB_ISTR_RESET) {
		// Reset request
		USB->ISTR = (uint16_t)~USB_ISTR_RESET;
		usbReset();
	}
	if (istr & USB_ISTR_SOF) {
		// Start of frame
		USB->ISTR = (uint16_t)~USB_ISTR_SOF;
	}
	if (istr & USB_ISTR_ESOF) {
		// Expected start of frame
		USB->ISTR = (uint16_t)~USB_ISTR_ESOF;
	}
}

void usbTransfer(uint16_t epid, uint16_t dir, const void *src, uint16_t dst, uint32_t size)
{
	if (usbStatus.dma.active)
		return;

	struct ep_t *ep = &eptable[epid][dir];
	ep->addr = dst;
	ep->count = size;

	if (size == 0) {
		volatile uint16_t *epr = &USB->EP0R + epid;
		uint32_t eprv = *epr;
		uint32_t epmasked = (eprv & (USB_EP_TYPE_MASK | USB_EP_KIND | USB_EPADDR_FIELD))
				| USB_EP_CTR_RX | USB_EP_CTR_TX;
		uint32_t mask = USB_EP0R_STAT_TX;
		if (dir == EP_RX)
			mask <<= 8;
		*epr = epmasked | (mask ^ (eprv & mask));
		return;
	}

	// Memory to memory, highest priority, increment mode
	// Memory size 16, peripheral size 32
	// Transfer complete interrupt enable
	DMA1_Channel1->CCR = DMA_CCR_MEM2MEM | DMA_CCR_PL |
			DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_1 |
			DMA_CCR_MINC | DMA_CCR_PINC |
			DMA_CCR_DIR | DMA_CCR_TCIE;
	DMA1_Channel1->CNDTR = size;
	DMA1_Channel1->CPAR = (uint32_t)USB_SYS_ADDR(dst);
	DMA1_Channel1->CMAR = (uint32_t)src;

	usbStatus.dma.epid = epid;
	usbStatus.dma.dir = dir;
	usbStatus.dma.active = 1;

	// Enable DMA
	DMA1_Channel1->CCR |= DMA_CCR_EN;
}

void DMA1_Channel1_IRQHandler()
{
	uint32_t epid = usbStatus.dma.epid;
	uint32_t dir = usbStatus.dma.dir;

	volatile uint16_t *epr = &USB->EP0R + epid;
	uint32_t eprv = *epr;
	uint32_t epmasked = (eprv & (USB_EP_TYPE_MASK | USB_EP_KIND | USB_EPADDR_FIELD))
			| USB_EP_CTR_RX | USB_EP_CTR_TX;

	uint32_t mask = USB_EP0R_STAT_TX;
	if (dir == EP_RX)
		mask <<= 8;
	*epr = epmasked | (mask ^ (eprv & mask));
	usbStatus.dma.active = 0;
	DMA1_Channel1->CCR &= ~DMA_CCR_EN;
	DMA1->IFCR = DMA_IFCR_CTCIF1;
}
