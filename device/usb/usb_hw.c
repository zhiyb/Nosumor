#include <device.h>
#include <macros.h>
#include <debug.h>
#include <system/systick.h>
#include <system/irq.h>
#include "usb_hw_macros.h"
#include "usb_core.h"
#include "usb_hw.h"

#define USB	USB_OTG_HS

static inline void usb_hw_init_gpio()
{
	// Enable IO compensation cell
	if (!(SYSCFG->CMPCR & SYSCFG_CMPCR_READY)) {
		RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
		SYSCFG->CMPCR |= SYSCFG_CMPCR_CMP_PD;
	}

	// Configure GPIOs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;

	// AF10: OTG2_HS
	// PA3, PA5, PB0, PB1, PB5, PB10, PB11, PB12, PB13, PC0, PC2, PC3

	// PA3	D0	IO
	GPIO_MODER(GPIOA, 3, GPIO_MODER_ALTERNATE);
	GPIO_OTYPER_PP(GPIOA, 3);
	GPIO_OSPEEDR(GPIOA, 3, GPIO_OSPEEDR_HIGH);
	GPIO_AFRL(GPIOA, 3, 10);

	// PA5	CK	I
	GPIO_MODER(GPIOA, 5, GPIO_MODER_ALTERNATE);
	GPIO_AFRL(GPIOA, 5, 10);

	// PB0	D1	IO
	GPIO_MODER(GPIOB, 0, GPIO_MODER_ALTERNATE);
	GPIO_OTYPER_PP(GPIOB, 0);
	GPIO_OSPEEDR(GPIOB, 0, GPIO_OSPEEDR_HIGH);
	GPIO_AFRL(GPIOB, 0, 10);

	// PB1	D2	IO
	GPIO_MODER(GPIOB, 1, GPIO_MODER_ALTERNATE);
	GPIO_OTYPER_PP(GPIOB, 1);
	GPIO_OSPEEDR(GPIOB, 1, GPIO_OSPEEDR_HIGH);
	GPIO_AFRL(GPIOB, 1, 10);

	// PB5	D7	IO
	GPIO_MODER(GPIOB, 5, GPIO_MODER_ALTERNATE);
	GPIO_OTYPER_PP(GPIOB, 5);
	GPIO_OSPEEDR(GPIOB, 5, GPIO_OSPEEDR_HIGH);
	GPIO_AFRL(GPIOB, 5, 10);

	// PB10	D3	IO
	GPIO_MODER(GPIOB, 10, GPIO_MODER_ALTERNATE);
	GPIO_OTYPER_PP(GPIOB, 10);
	GPIO_OSPEEDR(GPIOB, 10, GPIO_OSPEEDR_HIGH);
	GPIO_AFRH(GPIOB, 10, 10);

	// PB11	D4	IO
	GPIO_MODER(GPIOB, 11, GPIO_MODER_ALTERNATE);
	GPIO_OTYPER_PP(GPIOB, 11);
	GPIO_OSPEEDR(GPIOB, 11, GPIO_OSPEEDR_HIGH);
	GPIO_AFRH(GPIOB, 11, 10);

	// PB12	D5	IO
	GPIO_MODER(GPIOB, 12, GPIO_MODER_ALTERNATE);
	GPIO_OTYPER_PP(GPIOB, 12);
	GPIO_OSPEEDR(GPIOB, 12, GPIO_OSPEEDR_HIGH);
	GPIO_AFRH(GPIOB, 12, 10);

	// PB13	D6	IO
	GPIO_MODER(GPIOB, 13, GPIO_MODER_ALTERNATE);
	GPIO_OTYPER_PP(GPIOB, 13);
	GPIO_OSPEEDR(GPIOB, 13, GPIO_OSPEEDR_HIGH);
	GPIO_AFRH(GPIOB, 13, 10);

	// PC0	STP	O
	GPIO_MODER(GPIOC, 0, GPIO_MODER_ALTERNATE);
	GPIO_OTYPER_PP(GPIOC, 0);
	GPIO_OSPEEDR(GPIOC, 0, GPIO_OSPEEDR_HIGH);
	GPIO_AFRL(GPIOC, 0, 10);

	// PC2	DIR	I
	GPIO_MODER(GPIOC, 2, GPIO_MODER_ALTERNATE);
	GPIO_AFRL(GPIOC, 2, 10);

	// PC3	NXT	I
	GPIO_MODER(GPIOC, 3, GPIO_MODER_ALTERNATE);
	GPIO_AFRL(GPIOC, 3, 10);

	// Wait for IO compensation cell
	while (!(SYSCFG->CMPCR & SYSCFG_CMPCR_READY));
}

static inline void usb_hw_init_hs()
{
	// Enable OTG HS
	RCC->AHB1ENR |= RCC_AHB1ENR_OTGHSULPIEN_Msk | RCC_AHB1ENR_OTGHSEN_Msk;
	// Wait for AHB bus transactions
	while (!(USB->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL_Msk));
	// Core reset
	USB->GRSTCTL |= USB_OTG_GRSTCTL_CSRST_Msk;
	while (USB->GRSTCTL & USB_OTG_GRSTCTL_CSRST_Msk);
	// Reset PHY clock
	PCGCCTL(USB) = 0;
	USB->GOTGCTL = USB_OTG_GOTGCTL_OTGVER;
	USB->GLPMCFG = USB_OTG_GLPMCFG_ENBESL_Msk | USB_OTG_GLPMCFG_LPMEN_Msk;
	// OTG version 1.3 is obsolete, select version 2.0
	USB->GOTGCTL = USB_OTG_GOTGCTL_OTGVER;
	// Enable LPM errata behaviour, L1 deep/shallow sleep enable, LPM disable
	USB->GLPMCFG = USB_OTG_GLPMCFG_ENBESL_Msk | /*USB_OTG_GLPMCFG_LPMEN_Msk |*/
			USB_OTG_GLPMCFG_L1DSEN_Msk | USB_OTG_GLPMCFG_L1SSEN_Msk;
	// VBUS detection disabled, USB FS transceiver disabled
	USB->GCCFG = 0;
	// Force device mode, TRDT = 9, HNP and SRP capable, external ULPI HS PHY
	USB->GUSBCFG = USB_OTG_GUSBCFG_FDMOD_Msk | (1ul << 4 /* ULPISEL */) |
			USB_OTG_GUSBCFG_HNPCAP_Msk | USB_OTG_GUSBCFG_SRPCAP_Msk |
			(9 << USB_OTG_GUSBCFG_TRDT_Pos) | (4 << USB_OTG_GUSBCFG_TOCAL_Pos);
	// DMA enable, AHB burst 32 bytes
	USB->GAHBCFG = USB_OTG_GAHBCFG_DMAEN_Msk | (5 << USB_OTG_GAHBCFG_HBSTLEN_Pos);
	// Clear interrupts
	USB->GINTSTS = 0xffffffff;
	// Enable mode mismatch interrupt
	USB->GINTMSK = /*USB_OTG_GINTMSK_OTGINT |*/ USB_OTG_GINTMSK_MMISM;
	// Unmask interrupts
	USB->GAHBCFG |= USB_OTG_GAHBCFG_GINT_Msk /* GINTMSK */;

	// Setup NVIC interrupts
	uint32_t pg = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(OTG_HS_IRQn,
			 NVIC_EncodePriority(pg, NVIC_PRIORITY_USB, 1));
	NVIC_SetPriority(OTG_HS_WKUP_IRQn,
			 NVIC_EncodePriority(pg, NVIC_PRIORITY_USB, 0));
	NVIC_SetPriority(OTG_HS_EP1_IN_IRQn,
			 NVIC_EncodePriority(pg, NVIC_PRIORITY_USB_H, 1));
	NVIC_SetPriority(OTG_HS_EP1_OUT_IRQn,
			 NVIC_EncodePriority(pg, NVIC_PRIORITY_USB_H, 0));
	NVIC_EnableIRQ(OTG_HS_IRQn);
	//NVIC_EnableIRQ(OTG_HS_WKUP_IRQn);
	NVIC_EnableIRQ(OTG_HS_EP1_IN_IRQn);
	NVIC_EnableIRQ(OTG_HS_EP1_OUT_IRQn);
}

static inline void usb_hw_init_device()
{
	USB_OTG_DeviceTypeDef *dev = DEV(USB);
	// Disconnect USB
	usb_hw_connect(0);
	systick_delay(1);

	// Allocate 25% for iso IN DMA, enable transceiver delay, enumerate HS
	dev->DCFG = (0b00 << USB_OTG_DCFG_PERSCHIVL_Pos) | (1ul << 14 /* XCVRDLY */);
#if 0
	// Data transfer threshold
	dev->DTHRCTL = (32u << USB_OTG_DTHRCTL_RXTHRLEN_Pos) | (32u << USB_OTG_DTHRCTL_TXTHRLEN_Pos) |
			USB_OTG_DTHRCTL_RXTHREN_Msk | USB_OTG_DTHRCTL_ISOTHREN_Msk |
			USB_OTG_DTHRCTL_NONISOTHREN_Msk | USB_OTG_DTHRCTL_ARPEN_Msk;
#else
	// Disable thresholding
	dev->DTHRCTL = 0;
#endif
	// Interrupt and event masks
	// OUT: Transfer complete, setup done, status phase
	dev->DOEPMSK = USB_OTG_DOEPMSK_XFRCM_Msk |
			USB_OTG_DOEPMSK_STUPM_Msk | USB_OTG_DOEPMSK_OTEPSPRM_Msk /* STSPHSRXM */;
	// IN: Transfer complete, timeout
	dev->DIEPMSK = USB_OTG_DIEPMSK_XFRCM_Msk | USB_OTG_DIEPMSK_TOM_Msk;
	// Global: Reset, suspend, enumeration, OUT and IN endpoints
	USB->GINTMSK |= USB_OTG_GINTMSK_USBRST_Msk | USB_OTG_GINTMSK_USBSUSPM_Msk |
			USB_OTG_GINTMSK_ENUMDNEM_Msk |
			USB_OTG_GINTMSK_OEPINT_Msk | USB_OTG_GINTMSK_IEPINT_Msk;
}

static void usb_hw_init()
{
	usb_hw_init_gpio();
	usb_hw_init_hs();
	//usb_hs_irq_init(usb);
	usb_hw_init_device();
}

INIT_HANDLER(&usb_hw_init);

void usb_hw_connect(uint32_t e)
{
	USB_OTG_DeviceTypeDef *dev = DEV(USB);
	dev->DCTL = e ? 0 : USB_OTG_DCTL_SDIS_Msk;
}

uint32_t usb_hw_connected()
{
	USB_OTG_DeviceTypeDef *dev = DEV(USB);
	return !(dev->DCTL & USB_OTG_DCTL_SDIS_Msk);
}

void OTG_HS_IRQHandler()
{
	USB_OTG_DeviceTypeDef *dev = DEV(USB);
	uint32_t ists = USB->GINTSTS;
	uint32_t dsts = dev->DSTS;
	ists &= USB->GINTMSK;

	// Return if no interrupt to process
	if (!ists)
		return;
	uint32_t processed = 0;

#ifndef BOOTLOADER
	// Missed IN isochronous transfer frame
	if (ists & USB_OTG_GINTSTS_IISOIXFR_Msk) {
		USB->GINTSTS = USB_OTG_GINTSTS_IISOIXFR_Msk;
	}

	// Missed OUT isochronous transfer frame
	if (ists & USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Msk) {
		USB->GINTSTS = USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Msk;
	}
#endif

	// IN endpoint events
	if (ists & (USB_OTG_GINTSTS_OEPINT_Msk | USB_OTG_GINTSTS_IEPINT_Msk)) {
	}

	// OUT endpoint events
	if (ists & (USB_OTG_GINTSTS_OEPINT_Msk | USB_OTG_GINTSTS_IEPINT_Msk)) {
	}

#if 0
	// OTG event
	if (ists & USB_OTG_GINTSTS_OTGINT_Msk) {
	}
#endif

	// USB mode mismatch event
	if (ists & USB_OTG_GINTSTS_MMIS_Msk) {
		// Disconnect and panic
		usb_hw_connect(0);
		panic();
	}

	// USB suspend
	if (ists & (USB_OTG_GINTSTS_USBSUSP_Msk)) {
		USB->GINTSTS = USB_OTG_GINTSTS_USBSUSP_Msk;
		// Ignored for now
		processed = 1;
	}

	// USB reset
	if (ists & USB_OTG_GINTSTS_USBRST_Msk) {
		usb_core_reset();
		USB->GINTSTS = USB_OTG_GINTSTS_USBRST_Msk;
		processed = 1;
	}

	// USB emulation done
	if (ists & USB_OTG_GINTSTS_ENUMDNE_Msk) {
		usb_core_enumeration(DSTS_ENUM_SPD(dev->DSTS));
		USB->GINTSTS = USB_OTG_GINTSTS_ENUMDNE_Msk;
		processed = 1;
	}

	// Unimplemented interrupt
	if (!processed)
		dbgprintf(ESC_WARNING "%lu\tusb_hw: Unknown interrupt: 0x%08lx\n", systick_cnt(), ists);
}
