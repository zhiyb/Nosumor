#include <stm32f7xx.h>
#include <macros.h>
#include <debug.h>
#include "usb_macros.h"
#include "usb_irq.h"
#include "usb_ep0.h"
#include "usb.h"

static inline void usb_hs_init_gpio();

void usb_init(usb_t *usb, USB_OTG_GlobalTypeDef *base)
{
	usb->base = base;
	usb->epcnt[USB_EPIN] = 0;
	usb->epcnt[USB_EPOUT] = 0;
	usb_ep0_register(usb);
	if (base != USB_OTG_HS)
		return;
	usb_hs_init_gpio();

	// Enable OTG HS
	RCC->AHB1ENR |= RCC_AHB1ENR_OTGHSULPIEN | RCC_AHB1ENR_OTGHSEN;
	base->GCCFG = 0;
	base->GUSBCFG = USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_ULPIAR |
			USB_OTG_GUSBCFG_HNPCAP | USB_OTG_GUSBCFG_SRPCAP |
			(9 << USB_OTG_GUSBCFG_TRDT_Pos) |
			(4 << USB_OTG_GUSBCFG_TOCAL_Pos) | (1ul << 4);
	// Core reset
	base->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;
	while (base->GRSTCTL & USB_OTG_GRSTCTL_CSRST);
	base->GOTGCTL = USB_OTG_GOTGCTL_OTGVER;
	base->GAHBCFG = USB_OTG_GAHBCFG_PTXFELVL | USB_OTG_GAHBCFG_TXFELVL |
			(5 /* 8x 32-bit */ << USB_OTG_GAHBCFG_HBSTLEN_Pos) |
			USB_OTG_GAHBCFG_GINT;
	usb_hs_irq_init(usb);

	uint32_t pg = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(OTG_HS_IRQn, NVIC_EncodePriority(pg, 2, 1));
	NVIC_SetPriority(OTG_HS_WKUP_IRQn, NVIC_EncodePriority(pg, 2, 1));
	NVIC_SetPriority(OTG_HS_EP1_IN_IRQn, NVIC_EncodePriority(pg, 2, 0));
	NVIC_SetPriority(OTG_HS_EP1_OUT_IRQn, NVIC_EncodePriority(pg, 2, 0));
	NVIC_EnableIRQ(OTG_HS_IRQn);
	NVIC_EnableIRQ(OTG_HS_WKUP_IRQn);
	NVIC_EnableIRQ(OTG_HS_EP1_IN_IRQn);
	NVIC_EnableIRQ(OTG_HS_EP1_OUT_IRQn);
}

static inline void usb_hs_init_gpio()
{
	// MCO1: HSE / 1
	RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_MCO1 | RCC_CFGR_MCO1PRE)) |
			(0b10 << RCC_CFGR_MCO1_Pos) | (0 << RCC_CFGR_MCO1PRE_Pos);
	// Configure GPIOs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
	// 10: Alternative function mode
	GPIO_MODER(GPIOA, 8, 0b10);
	GPIO_OTYPER_PP(GPIOA, 8);
	GPIO_OSPEEDR(GPIOA, 8, 0b01);	// Medium speed (25MHz)
	// AF0: MCO1
	GPIO_AFRH(GPIOA, 8, 0);

	// Enable IO compensation cell
	if (!(SYSCFG->CMPCR & SYSCFG_CMPCR_READY)) {
		SYSCFG->CMPCR |= SYSCFG_CMPCR_CMP_PD;
		RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	}
	// PA3, PA5, PB0, PB1, PB5, PB10, PB11, PB12, PB13, PC0, PC2, PC3
	GPIO_MODER(GPIOA, 3, 0b10);	// D0	IO
	GPIO_OTYPER_PP(GPIOA, 3);	// Output push-pull
	GPIO_OSPEEDR(GPIOA, 3, 0b10);	// High speed (50~100MHz)
	GPIO_AFRL(GPIOA, 3, 10);	// AF10: OTG2_HS

	GPIO_MODER(GPIOA, 5, 0b10);	// CK	I
	GPIO_AFRL(GPIOA, 5, 10);

	GPIO_MODER(GPIOB, 0, 0b10);	// D1	IO
	GPIO_OTYPER_PP(GPIOB, 0);
	GPIO_OSPEEDR(GPIOB, 0, 0b10);
	GPIO_AFRL(GPIOB, 0, 10);

	GPIO_MODER(GPIOB, 1, 0b10);	// D2	IO
	GPIO_OTYPER_PP(GPIOB, 1);
	GPIO_OSPEEDR(GPIOB, 1, 0b10);
	GPIO_AFRL(GPIOB, 1, 10);

	GPIO_MODER(GPIOB, 5, 0b10);	// D7	IO
	GPIO_OTYPER_PP(GPIOB, 5);
	GPIO_OSPEEDR(GPIOB, 5, 0b10);
	GPIO_AFRL(GPIOB, 5, 10);

	GPIO_MODER(GPIOB, 10, 0b10);	// D3	IO
	GPIO_OTYPER_PP(GPIOB, 10);
	GPIO_OSPEEDR(GPIOB, 10, 0b10);
	GPIO_AFRH(GPIOB, 10, 10);

	GPIO_MODER(GPIOB, 11, 0b10);	// D4	IO
	GPIO_OTYPER_PP(GPIOB, 11);
	GPIO_OSPEEDR(GPIOB, 11, 0b10);
	GPIO_AFRH(GPIOB, 11, 10);

	GPIO_MODER(GPIOB, 12, 0b10);	// D5	IO
	GPIO_OTYPER_PP(GPIOB, 12);
	GPIO_OSPEEDR(GPIOB, 12, 0b10);
	GPIO_AFRH(GPIOB, 12, 10);

	GPIO_MODER(GPIOB, 13, 0b10);	// D6	IO
	GPIO_OTYPER_PP(GPIOB, 13);
	GPIO_OSPEEDR(GPIOB, 13, 0b10);
	GPIO_AFRH(GPIOB, 13, 10);

	GPIO_MODER(GPIOC, 0, 0b10);	// STP	O
	GPIO_OTYPER_PP(GPIOC, 0);
	GPIO_OSPEEDR(GPIOC, 0, 0b10);
	GPIO_AFRL(GPIOC, 0, 10);

	GPIO_MODER(GPIOC, 2, 0b10);	// DIR	I
	GPIO_AFRL(GPIOC, 2, 10);

	GPIO_MODER(GPIOC, 3, 0b10);	// NXT	I
	GPIO_AFRL(GPIOC, 3, 10);

	// Wait for IO compensation cell
	while (!(SYSCFG->CMPCR & SYSCFG_CMPCR_READY));
}

int usb_mode(usb_t *usb)
{
	return !!(usb->base->GINTSTS & USB_OTG_GINTSTS_CMOD);
}

void usb_init_device(usb_t *usb)
{
	USB_OTG_GlobalTypeDef *base = usb->base;
	USB_OTG_DeviceTypeDef *dev = DEVICE(base);
	// Reset PHY clock
	PCGCCTL(base) = 0;
	dev->DCFG = 0;	// Receive OUT packet, High speed
	base->GINTMSK |= USB_OTG_GINTMSK_USBRST | USB_OTG_GINTMSK_ENUMDNEM | USB_OTG_GINTMSK_RXFLVLM;
	dev->DOEPMSK = USB_OTG_DOEPMSK_XFRCM | USB_OTG_DOEPMSK_STUPM;
	dev->DIEPMSK = USB_OTG_DIEPMSK_XFRCM | USB_OTG_DIEPMSK_TOM;
	dev->DCTL = 0;	// Connect device
}
