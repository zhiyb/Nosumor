#include <stdint.h>
#include "stm32f1xx.h"
#include "keyboard.h"
#include "macros.h"
#include "escape.h"
#include "usb.h"
#include "usb_def.h"
#include "usb_ep0.h"
#include "usb_class.h"
#include "usb_debug.h"

struct status_t usbStatus;
struct ep_t eptable[8][2] USBTABLE;

void initUSB()
{
	usbStatus.dma.active = 0;
	uint32_t prioritygroup = NVIC_GetPriorityGrouping();

	// Enable clock and NVIC
	RCC->APB1ENR |= RCC_APB1ENR_USBEN;
	NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, NVIC_EncodePriority(prioritygroup, 2, 1));
	NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, NVIC_EncodePriority(prioritygroup, 2, 1));
	NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
	NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

	// Using DMA to transfer data between SRAM and USBSRAM
	// Memory to memory, medium priority, increment mode
	// Memory size 16, peripheral size 32
	// Transfer complete interrupt enable
	DMA1_Channel1->CCR = DMA_CCR_MEM2MEM | DMA_CCR_PL_0 |
			DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_1 |
			DMA_CCR_MINC | DMA_CCR_PINC |
			DMA_CCR_DIR | DMA_CCR_TCIE;
	NVIC_SetPriority(DMA1_Channel1_IRQn, NVIC_EncodePriority(prioritygroup, 2, 0));
	NVIC_EnableIRQ(DMA1_Channel1_IRQn);

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
	usbEP0Init();
	usbClassInit();
}

uint32_t usbConfigured()
{
	return usbStatus.config;
}

static void usbReset()
{
	usbStatus.addr = 0;
	usbStatus.config = 0;
	usbEP0Reset();
	usbClassReset();
	// Start USB device
	USB->DADDR = USB_DADDR_EF;
}

static void usbCTR()
{
	uint16_t epid = USB->ISTR & USB_ISTR_EP_ID;
	uint16_t dir = !!(USB->ISTR & USB_ISTR_DIR);
	struct ep_t *ep = &eptable[epid][dir];

	volatile uint16_t *epr = &USB_EPnR(epid);
	uint16_t eprv = *epr;
	uint16_t epmasked = (eprv & (USB_EP_TYPE_MASK | USB_EP_KIND | USB_EPADDR_FIELD))
			| USB_EP_CTR_RX | USB_EP_CTR_TX;

	if (eprv & USB_EP_CTR_RX) {
		if (eprv & USB_EP_SETUP) {
			writeString("\n<SETUP>");
			if (epid == 0)
				usbEP0Setup();
			else {
				// Unimplemented
				usbStall(epid, EP_TX);
				dbbkpt();
			}
		} else if ((ep->count & USB_RX_COUNT_MASK) != 0)
			dbbkpt();
		usbValid(epid, dir);
	}
	if (eprv & USB_EP_CTR_TX) {
		//writeString("\n<CTX>");
		if (usbStatus.addr) {
			USB->DADDR = usbStatus.addr | USB_DADDR_EF;
			usbStatus.addr = 0;
			writeString("<ADDR>");
		}
		if (usbStatus.dma.active && usbStatus.dma.size &&
				usbStatus.dma.epid == epid && usbStatus.dma.dir == dir) {
			uint8_t size = MIN(usbStatus.dma.size, usbStatus.dma.buffSize);
			ep->count = size;
			usbStatus.dma.size -= size;
			DMA1_Channel1->CNDTR = size;
			DMA1_Channel1->CMAR = (uint32_t)usbStatus.dma.ptr;
			usbStatus.dma.active = 1;
			DMA1_Channel1->CCR |= DMA_CCR_EN;	// Enable DMA
			usbStatus.dma.ptr = (uint8_t *)usbStatus.dma.ptr + size;
		}
		*epr = epmasked & ~USB_EP_CTR_TX;
	}
	if (!(eprv & (USB_EP_CTR_RX | USB_EP_CTR_TX)))
		dbbkpt();
	USB->ISTR = (uint16_t)~USB_ISTR_CTR;
}

void USB_HP_CAN1_TX_IRQHandler()
{
	//USB_TypeDef *usb = USB;
	dbbkpt();
}

void USB_LP_CAN1_RX0_IRQHandler()
{
	uint16_t istr;
	//writeChar('*');

	// Correct transfer
	while ((istr = USB->ISTR) & USB_ISTR_CTR)
		usbCTR();

	if (istr & USB_ISTR_PMAOVR) {
		// Packet memory overrun / underrun
		writeString("\n<PMA>");
		dbbkpt();
		USB->ISTR = (uint16_t)~USB_ISTR_PMAOVR;
	}
#if 0
	if (istr & USB_ISTR_ERR) {
		// Error
		USB->ISTR = (uint16_t)~USB_ISTR_ERR;
		writeString("\n<ERR>");
	}
#endif
	if (istr & USB_ISTR_WKUP) {
		// Wakeup request
		switch (USB->FNR & (USB_FNR_RXDP | USB_FNR_RXDM)) {
		case USB_FNR_RXDP:
		case USB_FNR_RXDP | USB_FNR_RXDM:
			// Possibly noise, go back to suspend
			USB->CNTR |= USB_CNTR_FSUSP | USB_CNTR_LP_MODE;
			writeString("\n<W-SUSP>");
			break;
		default:
			USB->CNTR &= ~USB_CNTR_FSUSP;
			writeString("\n<WKUP>");
		}
		USB->ISTR = (uint16_t)~USB_ISTR_WKUP;
	}
	if (istr & USB_ISTR_SUSP) {
		// Suspend request
		USB->ISTR = (uint16_t)~USB_ISTR_SUSP;
		USB->CNTR |= USB_CNTR_FSUSP | USB_CNTR_LP_MODE;
		writeString("\n<SUSP>");
	}
	if (istr & USB_ISTR_RESET) {
		// Reset request
		USB->ISTR = (uint16_t)~USB_ISTR_RESET;
		usbReset();
		writeString("\n" ESC_GREY "<RESET>" ESC_DEFAULT);
	}
#if 0
	if (istr & USB_ISTR_SOF) {
		writeChar('S');
		// Start of frame
		USB->ISTR = (uint16_t)~USB_ISTR_SOF;
	}
	if (istr & USB_ISTR_ESOF) {
		writeChar('E');
		dumpHex((USB->FNR & USB_FNR_LSOF) >> 11);
		// Expected start of frame
		USB->ISTR = (uint16_t)~USB_ISTR_ESOF;
	}
#endif
}

static inline uint32_t usbIsBusy(uint16_t epid, uint16_t dir)
{
	volatile uint16_t *epr = &USB_EPnR(epid);
	uint16_t mask = dir != EP_TX ? USB_EPRX_STAT : USB_EPTX_STAT;
	uint16_t valid = dir != EP_TX ? USB_EP_RX_VALID : USB_EP_TX_VALID;
	if (usbStatus.dma.epid == epid)
		if (usbStatus.dma.active)
			return 1;
	return (*epr & mask) == valid;
}

void usbTransfer(uint8_t epid, uint8_t dir, uint8_t buffSize, uint8_t size, const void *ptr)
{
	if (size == 0)
		return usbTransferEmpty(epid, dir);

	struct ep_t *ep = &eptable[epid][dir];
	do
		if (USB->CNTR & USB_CNTR_FSUSP)
			return;
	while (usbIsBusy(epid, dir));

	while (usbStatus.dma.active);
	usbStatus.dma.epid = epid;
	usbStatus.dma.dir = dir;
	usbStatus.dma.buffSize = buffSize;
	ep->count = buffSize = MIN(size, buffSize);
	usbStatus.dma.size = size - buffSize;

	DMA1_Channel1->CNDTR = buffSize;
	DMA1_Channel1->CPAR = (uint32_t)USB_SYS_ADDR(ep->addr);
	DMA1_Channel1->CMAR = (uint32_t)ptr;
	usbStatus.dma.active = 1;
	DMA1_Channel1->CCR |= DMA_CCR_EN;	// Enable DMA
	usbStatus.dma.ptr = (uint8_t *)ptr + buffSize;
}

void usbTransferEmpty(uint16_t epid, uint16_t dir)
{
	struct ep_t *ep = &eptable[epid][dir];
	do
		if (USB->CNTR & USB_CNTR_FSUSP)
			return;
	while (usbIsBusy(epid, dir));
	ep->count = 0;
	usbValid(epid, dir);
}

void usbHandshake(uint16_t epid, uint16_t dir, uint16_t type)
{
	volatile uint16_t *epr = &USB_EPnR(epid);
	uint16_t eprv = *epr;
	uint16_t epmasked = (eprv & (USB_EP_TYPE_MASK | USB_EP_KIND | USB_EPADDR_FIELD))
			| USB_EP_CTR_RX | USB_EP_CTR_TX;
	uint16_t mask = USB_EPTX_STAT;
	if (dir != EP_TX) {
		mask <<= 8;
		type <<= 8;
		epmasked &= ~USB_EP_CTR_RX;
	}
	*epr = epmasked | (type ^ (eprv & mask));

#if 0
	writeChar('<');
	if (dir != EP_TX) {
		writeString("RX>");
		return;
	} else {
		writeString("TX");
		dumpHex(epid);
	}
	switch (type) {
	case USB_EP_TX_DIS:
		writeString("D>");
		break;
	case USB_EP_TX_STALL:
		writeString("S>");
		break;
	case USB_EP_TX_NAK:
		writeString("N>");
		break;
	case USB_EP_TX_VALID:
		writeString("V>");
	}
#endif
}

void DMA1_Channel1_IRQHandler()
{
	usbValid(usbStatus.dma.epid, usbStatus.dma.dir);
	usbStatus.dma.active = usbStatus.dma.size;
	DMA1_Channel1->CCR &= ~DMA_CCR_EN;
	DMA1->IFCR = DMA_IFCR_CTCIF1;
}
