#include <string.h>
#include <device.h>
#include <module.h>
#include <debug.h>
#include <irq.h>
#include "usb_macros.h"
#include "usb_ep.h"
#include "usb_ep0.h"

#if 0
#include "usb_macros.h"
#include "usb_irq.h"
#include "usb_ep0.h"
#include "usb_structs.h"
#include "usb.h"
#endif

static void usb_hs_init_gpio();

static void usb_init(USB_OTG_GlobalTypeDef *base)
{
#if 0
	memset(usb, 0, sizeof(usb_t));
	usb->base = base;
#endif

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
	// Select ULPI interface
	base->GUSBCFG = USB_OTG_GUSBCFG_FDMOD_Msk | USB_OTG_GUSBCFG_ULPIIPD_Msk |
			USB_OTG_GUSBCFG_HNPCAP_Msk | USB_OTG_GUSBCFG_SRPCAP_Msk |
			(9 << USB_OTG_GUSBCFG_TRDT_Pos) |
			(4 << USB_OTG_GUSBCFG_TOCAL_Pos) | (1ul << 4 /* ULPISEL */);
	base->GAHBCFG = (7 /* 16x 32-bit */ << USB_OTG_GAHBCFG_HBSTLEN_Pos) |
			USB_OTG_GAHBCFG_GINT_Msk | USB_OTG_GAHBCFG_DMAEN_Msk;
	// Interrupt masks
	base->GINTSTS = 0xffffffff;
	base->GINTMSK = USB_OTG_GINTMSK_OTGINT | USB_OTG_GINTMSK_MMISM;
	//usb_hs_irq_init(usb);

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

static void usb_hs_init_gpio()
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

	// Enable clock output to PHY
	MODULE_MSG(module_init, "mco1", 1);
}

static uint32_t usb_mode(USB_OTG_GlobalTypeDef *base)
{
	return !!(base->GINTSTS & USB_OTG_GINTSTS_CMOD);
}

static void usb_connect(USB_OTG_GlobalTypeDef *base, int e)
{
	USB_OTG_DeviceTypeDef *dev = DEV(base);
	if (!e)	{
		// Disconnect device
		dev->DCTL = USB_OTG_DCTL_SDIS_Msk;
#if 0
		usb_disable(usb);
		systick_delay(100);
		MODULE_MSG(module_init, "tick.delay", 100);
#endif
	} else
		dev->DCTL = 0;	// Connect device
}

static void usb_init_device(USB_OTG_GlobalTypeDef *base)
{
	//USB_OTG_GlobalTypeDef *base = usb->base;
	USB_OTG_DeviceTypeDef *dev = DEV(base);
	// Disconnect USB
	usb_connect(base, 0);
#if 0
	usb_ep0_register(usb);
#endif
	// Enable transceiver delay
	dev->DCFG = (0b10 << USB_OTG_DCFG_PERSCHIVL_Pos) | (1ul << 14u) |
			USB_OTG_DCFG_NZLSOHSK_Msk;
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
	dev->DOEPMSK = USB_OTG_DOEPMSK_XFRCM_Msk | USB_OTG_DOEPMSK_STUPM_Msk | USB_OTG_DOEPMSK_OTEPSPRM_Msk;
	dev->DIEPMSK = USB_OTG_DIEPMSK_XFRCM_Msk | USB_OTG_DIEPMSK_TOM_Msk;
	base->GINTMSK |= USB_OTG_GINTMSK_USBRST_Msk | USB_OTG_GINTMSK_USBSUSPM_Msk |
			USB_OTG_GINTMSK_ENUMDNEM_Msk |
			USB_OTG_GINTMSK_OEPINT_Msk | USB_OTG_GINTMSK_IEPINT_Msk;
	// Register endpoint 0
	usb_ep0_reserve();
}

static void usb_start(USB_OTG_GlobalTypeDef *base)
{
	// Only device mode is supported
	while (usb_mode(base) != 0);
	dbgprintf(ESC_INFO "[USB.Core] Starting in " ESC_DATA "%s" ESC_INFO " mode\n",
	       usb_mode(base) ? "host" : "device");
	usb_init_device(base);
}

static void usb_active(USB_OTG_GlobalTypeDef *base)
{
	MODULE_MSG(module_init, "tick.delay", 5);
	usb_ep_reserve();
	//usb_connect(base, 1);
}

#if 0
void usb_process(usb_t *usb)
{
	usb_ep0_process(usb);
}

uint32_t usb_active(usb_t *usb)
{
	uint32_t active = usb->active;
	uint32_t act = active != usb->act;
	usb->act = active;
	return act;
}
#endif

static void *callback(void *inst, uint32_t msg, void *data)
{
	UNUSED(data);
	if (msg == HASH("init")) {
		usb_init(inst);
		return 0;
	} else if (msg == HASH("start")) {
		usb_start(inst);
		return 0;
	} else if (msg == HASH("active")) {
		usb_active(inst);
		return 0;
	}
	return 0;
}

MODULE(usb, 0, USB_OTG_HS, callback);
