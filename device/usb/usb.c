#include <string.h>
#include <stm32f7xx.h>
#include "../macros.h"
#include "../systick.h"
#include "../irq.h"
#include "../debug.h"
#include "usb_macros.h"
#include "usb_irq.h"
#include "usb_ep0.h"
#include "usb_structs.h"
#include "usb.h"

static inline void usb_hs_init_gpio();

void usb_init(usb_t *usb, USB_OTG_GlobalTypeDef *base)
{
	memset(usb, 0, sizeof(usb_t));
	usb->base = base;

	if (base != USB_OTG_HS) {
		dbgbkpt();
		return;
	}
	usb_hs_init_gpio();

	// Enable OTG HS
	RCC->AHB1ENR |= RCC_AHB1ENR_OTGHSULPIEN_Msk | RCC_AHB1ENR_OTGHSEN_Msk;
	// Core reset
	while (!(base->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL_Msk));
	base->GRSTCTL |= USB_OTG_GRSTCTL_CSRST_Msk;
	while (base->GRSTCTL & USB_OTG_GRSTCTL_CSRST_Msk);
	// Reset PHY clock
	PCGCCTL(base) = 0;
	base->GOTGCTL = USB_OTG_GOTGCTL_OTGVER;
	base->GLPMCFG = USB_OTG_GLPMCFG_ENBESL_Msk | USB_OTG_GLPMCFG_LPMEN_Msk;
	base->GCCFG = 0;
	base->GUSBCFG = USB_OTG_GUSBCFG_FDMOD_Msk | USB_OTG_GUSBCFG_ULPIIPD_Msk |
			USB_OTG_GUSBCFG_HNPCAP_Msk | USB_OTG_GUSBCFG_SRPCAP_Msk |
			(9 << USB_OTG_GUSBCFG_TRDT_Pos) |
			(4 << USB_OTG_GUSBCFG_TOCAL_Pos) | (1ul << 4);
	base->GAHBCFG = (5 /* 8x 32-bit */ << USB_OTG_GAHBCFG_HBSTLEN_Pos) |
			USB_OTG_GAHBCFG_GINT_Msk | USB_OTG_GAHBCFG_DMAEN_Msk;
	// Interrupt masks
	base->GINTSTS = 0xffffffff;
	base->GINTMSK = USB_OTG_GINTMSK_OTGINT | USB_OTG_GINTMSK_MMISM;
	usb_hs_irq_init(usb);

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
	NVIC_EnableIRQ(OTG_HS_WKUP_IRQn);
	NVIC_EnableIRQ(OTG_HS_EP1_IN_IRQn);
	NVIC_EnableIRQ(OTG_HS_EP1_OUT_IRQn);
}

static inline void usb_hs_init_gpio()
{
	// Enable IO compensation cell
	if (!(SYSCFG->CMPCR & SYSCFG_CMPCR_READY)) {
		RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
		SYSCFG->CMPCR |= SYSCFG_CMPCR_CMP_PD;
	}
	// Configure GPIOs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
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
	USB_OTG_DeviceTypeDef *dev = DEV(base);
	// Disconnect USB
	usb_connect(usb, 0);
	usb_ep0_register(usb);
	// Enable transceiver delay
	dev->DCFG = (0b10 << USB_OTG_DCFG_PERSCHIVL_Pos) | (1ul << 14u) |
			USB_OTG_DCFG_NZLSOHSK_Msk;
	// DMA threshold
	dev->DTHRCTL = (32u << USB_OTG_DTHRCTL_RXTHRLEN_Pos) | (32u << USB_OTG_DTHRCTL_TXTHRLEN_Pos) |
			USB_OTG_DTHRCTL_RXTHREN_Msk | USB_OTG_DTHRCTL_ISOTHREN_Msk | USB_OTG_DTHRCTL_NONISOTHREN_Msk;
	dev->DOEPMSK = USB_OTG_DOEPMSK_XFRCM_Msk |
			USB_OTG_DOEPMSK_STUPM_Msk | USB_OTG_DOEPMSK_OTEPSPRM_Msk;
	dev->DIEPMSK = USB_OTG_DIEPMSK_XFRCM_Msk | USB_OTG_DIEPMSK_TOM_Msk;
	base->GINTMSK |= USB_OTG_GINTMSK_USBRST_Msk | USB_OTG_GINTMSK_USBSUSPM_Msk |
			USB_OTG_GINTMSK_ENUMDNEM_Msk |
			USB_OTG_GINTMSK_OEPINT_Msk | USB_OTG_GINTMSK_IEPINT_Msk;
}

void usb_connect(usb_t *usb, int e)
{
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	if (!e)	{
		// Disconnect device
		dev->DCTL = USB_OTG_DCTL_SDIS_Msk;
		usb_disable(usb);
		systick_delay(100);
	} else
		dev->DCTL = 0;	// Connect device
}

void usb_process(usb_t *usb)
{
	usb_ep0_process(usb);
}
