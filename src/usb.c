#include <stdint.h>
#include "stm32f1xx.h"
#include "keyboard.h"
#include "macros.h"
#include "usb.h"
#include "usb_def.h"
#include "debug.h"

extern uint32_t _susbram;

struct ep_t eptable[8][2] __attribute__((section(".usbtable")));

static __IO union eprx_t ep0rx __attribute__((section(".usbram")));
static uint32_t ep0tx[4] __attribute__((section(".usbram")));

void initUSB()
{
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
			USB_CNTR_WKUPM | USB_CNTR_SUSPM | USB_CNTR_RESETM |
			USB_CNTR_SOFM | USB_CNTR_ESOFM;
	eptable[0][EP_TX].addr = USB_LOCAL_ADDR(ep0tx);
	eptable[0][EP_TX].count = 0;
	eptable[0][EP_RX].addr = USB_LOCAL_ADDR(&ep0rx);
	eptable[0][EP_RX].count = USB_RX_COUNT(sizeof(ep0rx) / 2);
}

static void usbReset()
{
	// Configure endpoint 0
	USB->EP0R = USB_EP_CONTROL | USB_EP_RX_VALID | USB_EP_TX_VALID;
	// Start USB device
	USB->DADDR = USB_DADDR_EF;
}

static void usbCTR()
{
	USB_TypeDef *usb = USB;
	uint32_t epid = USB->ISTR & USB_ISTR_EP_ID;
	uint32_t dir = USB->ISTR & USB_ISTR_DIR ? 1 : 0;
	struct ep_t *ep = &eptable[epid][dir];

	volatile uint16_t *epr = &USB->EP0R + epid;
	uint32_t eprv = *epr;
	uint32_t epmasked = (eprv & (USB_EP_TYPE_MASK | USB_EP_KIND | USB_EPADDR_FIELD))
			| USB_EP_CTR_RX | USB_EP_CTR_TX;

	// 0xea60
	if (eprv & USB_EP_CTR_RX) {
		if (eprv & USB_EP_SETUP) {
			usbSetup(epid);
		}
		*epr = epmasked & ~USB_EP_CTR_RX;
	}
	if (eprv & USB_EP_CTR_TX) {
		*epr = epmasked & ~USB_EP_CTR_TX;
	}
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
	uint16_t istr = USB->ISTR;
	while (istr & USB_ISTR_CTR)	// Correct transfer
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
