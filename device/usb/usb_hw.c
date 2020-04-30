#include <device.h>
#include <macros.h>
#include <debug.h>
#include <system/systick.h>
#include <system/irq.h>
#include "usb_hw_macros.h"
#include "usb_hw.h"

#define USB		USB_OTG_HS
#define USB_EP_NUM	8
#define USB_RAM_SIZE	(4 * 1024)
#define DMA_RAM_SIZE	(8 * 1024)

#define EP_CONF_IN	1
#define EP_CONF_OUT	0

// Extra transfer type
#define EP_UNUSED	0xff

LIST(usb_reset, usb_basic_handler_t);
LIST(usb_susp, usb_basic_handler_t);
LIST(usb_enum, usb_enum_handler_t);

struct {
	uint16_t top, num;
} fifo = {0};

struct {
	struct {
		uint32_t type;
		uint32_t maxsize;
		uint32_t xfrsize;
		void *p;
		struct {
			usb_ep_irq_xfrc_handler_t xfrc;
		} func;
	} ep[USB_EP_NUM];
} ep_in = {0};

struct {
	struct {
		uint32_t type;
		uint32_t maxsize;
		uint32_t xfrsize;
		struct {
			usb_ep_irq_stup_handler_t stup;
			usb_ep_irq_stsp_handler_t stsp;
			usb_ep_irq_xfrc_handler_t xfrc;
		} func;
	} ep[USB_EP_NUM];
} ep_out = {0};

uint8_t ram[DMA_RAM_SIZE] SECTION(".dmaram");
void *pram;

// size in 32-bit words, returns FIFO number
static uint32_t usb_hw_fifo_alloc(uint32_t size);

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
	// OTG version 1.3 is obsolete, select version 2.0
	USB->GOTGCTL = USB_OTG_GOTGCTL_OTGVER;
	USB->GLPMCFG = USB_OTG_GLPMCFG_ENBESL_Msk | USB_OTG_GLPMCFG_LPMEN_Msk;
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

	// Allocate 25% for iso IN DMA, enable transceiver delay, enumerate HS,
	// ignore zero-length status OUT packets
	dev->DCFG = (0b00 << USB_OTG_DCFG_PERSCHIVL_Pos) | (1ul << 14 /* XCVRDLY */) |
		    (1 << USB_OTG_DCFG_NZLSOHSK_Msk);
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

	// Call suspend on disconnect
	if (!e)
		LIST_ITERATE(usb_susp, usb_basic_handler_t, p) (*p)();
}

uint32_t usb_hw_connected()
{
	USB_OTG_DeviceTypeDef *dev = DEV(USB);
	return !(dev->DCTL & USB_OTG_DCTL_SDIS_Msk);
}

static void usb_hw_reset()
{
	USB_OTG_DeviceTypeDef *dev = DEV(USB);
	// Disable endpoint interrupts
	dev->DAINTMSK = 0;
	usb_hw_set_addr(0);
#if 0	// This doesn't work, probably because USBAEP is not set
	// Disable active endpoints
	for (int ep = 1; ep < USB_EP_NUM; ep++) {
		USB_OTG_INEndpointTypeDef *epin = EP_IN(USB, ep);
		USB_OTG_OUTEndpointTypeDef *epout = EP_OUT(USB, ep);
		if (epin->DIEPCTL & USB_OTG_DIEPCTL_EPENA_Msk) {
#if DEBUG
			printf(ESC_DEBUG "%lu\tusb_hw: " ESC_WRITE "EP %u IN" ESC_DEBUG " reset\n", systick_cnt(), ep);
#endif
			epin->DIEPCTL = ((epin->DIEPCTL) & (USB_OTG_DIEPCTL_TXFNUM_Msk | USB_OTG_DIEPCTL_MPSIZ_Msk |
					USB_OTG_DIEPCTL_EPTYP_Msk)) | USB_OTG_DIEPCTL_EPDIS_Msk;
		}
		if (epout->DOEPCTL & USB_OTG_DOEPCTL_EPENA_Msk) {
#if DEBUG
			printf(ESC_DEBUG "%lu\tusb_hw: " ESC_READ "EP %u OUT" ESC_DEBUG " reset\n", systick_cnt(), ep);
#endif
			epout->DOEPCTL = (epout->DOEPCTL & (USB_OTG_DOEPCTL_MPSIZ_Msk | USB_OTG_DOEPCTL_EPTYP_Msk)) |
					USB_OTG_DOEPCTL_EPDIS_Msk;
		}
		while (epin->DIEPCTL & USB_OTG_DIEPCTL_EPENA_Msk);
		while (epout->DOEPCTL & USB_OTG_DOEPCTL_EPENA_Msk);
	}
#endif
	// Reset endpoint status
	for (int ep = 0; ep < USB_EP_NUM; ep++) {
		ep_in.ep[ep].type = EP_UNUSED;
		ep_out.ep[ep].type = EP_UNUSED;
	}

	pram = &ram[0];

	fifo.top = 0;
	fifo.num = 0;

	// Allocate half of FIFO RAM to Rx
#if DEBUG >= 5
	if (usb_hw_fifo_alloc(USB_RAM_SIZE / 4 / 2) != (uint32_t)-1)
		USB_ERROR();
#else
	usb_hw_fifo_alloc(USB_RAM_SIZE / 2);
#endif
}

USB_RESET_HANDLER(&usb_hw_reset);

static uint32_t usb_hw_fifo_alloc(uint32_t size)
{
	uint32_t num  = fifo.num;
	uint32_t addr = fifo.top;
	fifo.num += 1;
	fifo.top += size;
#if DEBUG >= 5
	if (fifo.top > USB_RAM_SIZE / 4)
		USB_ERROR();
#endif
	switch (num) {
	case 0:		// Rx FIFO
		USB->GRXFSIZ = size << USB_OTG_GRXFSIZ_RXFD_Pos;
		break;
	case 1:		// Tx endpoint 0
		USB->DIEPTXF0_HNPTXFSIZ = DIEPTXF(addr, size);
		break;
	default:	// Other Tx endpoints
		USB->DIEPTXF[num - 2] = DIEPTXF(addr, size);
	}
#if DEBUG
	printf(ESC_DEBUG "%lu\tusb_hw: FIFO RAM allocated %lu, %u bytes\n",
			systick_cnt(), num, fifo.top);
#endif
	return num - 1;
}

void usb_hw_set_addr(uint8_t addr)
{
	DEV(USB)->DCFG = (DEV(USB)->DCFG & ~USB_OTG_DCFG_DAD_Msk) |
			((addr << USB_OTG_DCFG_DAD_Pos) & USB_OTG_DCFG_DAD_Msk);
}

static void usb_hw_ep0_alloc(uint32_t dir, uint32_t size)
{
	USB_OTG_DeviceTypeDef *dev = DEV(USB);
	uint32_t epsize = 0b00;
	switch (size) {
	case 64:
		epsize = 0b00;
		break;
	case 32:
		epsize = 0b01;
		break;
	case 16:
		epsize = 0b10;
		break;
	case 8:
		epsize = 0b11;
		break;
#if DEBUG >= 5
	default:
		USB_ERROR();
#endif
	}
	if (dir == EP_DIR_IN) {
		USB_OTG_INEndpointTypeDef *epin = EP_IN(USB, 0);
		uint32_t fifo = usb_hw_fifo_alloc(size / 4);
#if DEBUG >= 5
		if (fifo != 0)
			USB_ERROR();
#endif
		epin->DIEPCTL = (fifo << USB_OTG_DIEPCTL_TXFNUM_Pos) | (epsize << USB_OTG_DIEPCTL_MPSIZ_Pos);
	} else {
#if DEBUG >= 5
		USB_OTG_INEndpointTypeDef *epin = EP_IN(USB, 0);
		//USB_OTG_OUTEndpointTypeDef *epout = EP_OUT(USB, 0);
		if ((epin->DIEPCTL & USB_OTG_DIEPCTL_MPSIZ_Msk) >> USB_OTG_DIEPCTL_MPSIZ_Pos != epsize)
			USB_ERROR();
#endif
	}
}

uint32_t usb_hw_ep_alloc(usb_ep_type_t eptype, uint32_t dir, uint32_t type, uint32_t maxsize)
{
	USB_OTG_DeviceTypeDef *dev = DEV(USB);
	uint32_t epnum = 0;
	if (dir == EP_DIR_IN) {
		if (eptype == UsbEp0)
			epnum = 0;
		else if (eptype == UsbEpFast)
			epnum = 1;
		else
			epnum = 2;
		for (; ep_in.ep[epnum].type != EP_UNUSED; epnum++);
		USB_OTG_INEndpointTypeDef *epin = EP_IN(USB, epnum);
		if (epnum == 0) {
#if DEBUG >= 5
			if (type != EP_CONTROL)
				USB_ERROR();
#endif
			usb_hw_ep0_alloc(dir, maxsize);
		} else {
			uint32_t fifo = usb_hw_fifo_alloc((maxsize + 3) / 4);
			epin->DIEPCTL = (fifo << USB_OTG_DIEPCTL_TXFNUM_Pos) |
					DIEPCTL_EP_TYP(type) | (maxsize << USB_OTG_DIEPCTL_MPSIZ_Pos);
		}
		ep_in.ep[epnum].type = type;
		ep_in.ep[epnum].maxsize = maxsize;
		// Clear interrupts
		epin->DIEPINT = USB_OTG_DIEPINT_XFRC_Msk;
		// Unmask interrupts
		dev->DAINTMSK |= DAINTMSK_IN(epnum);
		dbgprintf(ESC_DEBUG "%lu\tusb_hw: Endpoint " ESC_WRITE "IN %lu"
				ESC_DEBUG " allocated\n", systick_cnt(), epnum);
	} else if (dir == EP_DIR_OUT) {
		if (eptype == UsbEp0)
			epnum = 0;
		else if (eptype == UsbEpFast)
			epnum = 1;
		else
			epnum = 2;
		for (; ep_out.ep[epnum].type != EP_UNUSED; epnum++);
		USB_OTG_OUTEndpointTypeDef *epout = EP_OUT(USB, epnum);
		if (epnum == 0) {
#if DEBUG >= 5
			if (type != EP_CONTROL)
				USB_ERROR();
#endif
			usb_hw_ep0_alloc(dir, maxsize);
		} else {
			epout->DOEPCTL = DIEPCTL_EP_TYP(type) | (maxsize << USB_OTG_DIEPCTL_MPSIZ_Pos);
		}
		ep_out.ep[epnum].type = type;
		ep_out.ep[epnum].maxsize = maxsize;
		ep_out.ep[epnum].xfrsize = 0;
		// Clear interrupts
		epout->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk | USB_OTG_DOEPINT_STUP_Msk |
				USB_OTG_DOEPINT_OTEPSPR_Msk;
		// Unmask interrupts
		dev->DAINTMSK |= DAINTMSK_OUT(epnum);
#if DEBUG
		printf(ESC_DEBUG "%lu\tusb_hw: Endpoint " ESC_READ "OUT %lu"
				ESC_DEBUG " allocated\n", systick_cnt(), epnum);
#endif
#if DEBUG >= 5
	} else {
		USB_ERROR();
#endif
	}
	return epnum;
}

uint32_t usb_hw_ep_max_size(uint32_t dir, uint32_t epnum)
{
	if (dir == EP_DIR_IN)
		return ep_in.ep[epnum].maxsize;
	else
		return ep_out.ep[epnum].maxsize;
}

void *usb_hw_ram_alloc(uint32_t size)
{
	void *p = pram;
	// Align to 4-byte boundary
	size = (size + 3) / 4 * 4;
	pram += size;
#if DEBUG
	if (pram - (void *)&ram[0] > DMA_RAM_SIZE)
		USB_ERROR();
#endif
	dbgprintf(ESC_DEBUG "%lu\tusb_hw: DMA RAM allocated %u bytes\n",
			systick_cnt(), pram - (void *)&ram[0]);
	return p;
}

void usb_hw_ep_out_irq(uint32_t epnum, usb_ep_irq_stup_handler_t stup,
		       usb_ep_irq_stsp_handler_t stsp, usb_ep_irq_xfrc_handler_t xfrc)
{
	ep_out.ep[epnum].func.stup = stup;
	ep_out.ep[epnum].func.stsp = stsp;
	ep_out.ep[epnum].func.xfrc = xfrc;
}

void usb_hw_ep_out(uint32_t epnum, void *p, uint32_t setup, uint32_t pkt, uint32_t size)
{
	USB_OTG_OUTEndpointTypeDef *epout = EP_OUT(USB, epnum);
	uint32_t epsize = ep_out.ep[epnum].maxsize;
	// Set transfer size to maximum packet size to be interrupted at the end of each packet
	if (size == 0)
		size = epsize;
	ep_out.ep[epnum].xfrsize = size;
	// Setup frame PID for ISOCHRONOUS
	if (ep_out.ep[epnum].type == EP_ISOCHRONOUS)
		USB_TODO();
	epout->DOEPDMA = (uint32_t)p;
	epout->DOEPTSIZ = (setup << USB_OTG_DOEPTSIZ_STUPCNT_Pos) | (pkt << USB_OTG_DOEPTSIZ_PKTCNT_Pos) |
			(size << USB_OTG_DOEPTSIZ_XFRSIZ_Pos);
	epout->DOEPCTL = (epout->DOEPCTL & (USB_OTG_DOEPCTL_MPSIZ_Msk | USB_OTG_DOEPCTL_EPTYP_Msk)) |
			USB_OTG_DOEPCTL_USBAEP_Msk | USB_OTG_DOEPCTL_EPENA_Msk | USB_OTG_DOEPCTL_CNAK_Msk;
	// Clear global NAK
	//dev->DCTL |= USB_OTG_DCTL_CGONAK_Msk;
}

void usb_hw_ep_out_nak(uint32_t epnum)
{
	USB_OTG_OUTEndpointTypeDef *epout = EP_OUT(USB, epnum);
	epout->DOEPCTL = (epout->DOEPCTL & (USB_OTG_DOEPCTL_MPSIZ_Msk | USB_OTG_DOEPCTL_EPTYP_Msk)) |
			USB_OTG_DOEPCTL_USBAEP_Msk | USB_OTG_DOEPCTL_SNAK_Msk;
#if DEBUG >= 6
	printf(ESC_DEBUG "%lu\tusb_hw: " ESC_READ "EP %lu OUT" ESC_DEBUG " NAK\n", systick_cnt(), epnum);
#endif
}

void usb_hw_ep_in(uint32_t epnum, void *p, uint32_t size, usb_ep_irq_xfrc_handler_t xfrc)
{
	USB_OTG_INEndpointTypeDef *epin = EP_IN(USB, epnum);
	uint32_t maxsize = ep_in.ep[epnum].maxsize;
#if DEBUG
	if (size > maxsize)
		USB_ERROR();
#endif
	uint32_t pcnt = 1;
	if (size != 0)
		pcnt = (size + maxsize - 1) / maxsize;
#if DEBUG
	if ((epin->DIEPCTL & USB_OTG_DIEPCTL_EPENA_Msk) && (epin->DIEPCTL & USB_OTG_DIEPCTL_USBAEP_Msk))
		USB_ERROR();
#endif
	ep_in.ep[epnum].p = p;
	ep_in.ep[epnum].xfrsize = size;
	ep_in.ep[epnum].func.xfrc = xfrc;
#if DEBUG >= 6
	USB_OTG_INEndpointTypeDef epinval = *EP_IN(USB, epnum);
	printf(ESC_DEBUG "%lu\tusb_hw: " ESC_WRITE "EP %lu IN" ESC_DEBUG " before 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n",
	       systick_cnt(), epnum, epinval.DIEPCTL, epinval.DIEPINT, epinval.DIEPTSIZ, epin->DIEPDMA);
	if (epinval.DIEPINT & USB_OTG_DIEPINT_XFRC_Msk)
		USB_TODO();
#endif
	epin->DIEPDMA = (uint32_t)p;
	epin->DIEPTSIZ = (pcnt << USB_OTG_DIEPTSIZ_PKTCNT_Pos) | (size << USB_OTG_DIEPTSIZ_XFRSIZ_Pos);
#if DEBUG >= 6
	epin->DIEPCTL = ((epin->DIEPCTL) & (USB_OTG_DIEPCTL_TXFNUM_Msk | USB_OTG_DIEPCTL_MPSIZ_Msk |
			USB_OTG_DIEPCTL_EPTYP_Msk)) | /*USB_OTG_DIEPCTL_CNAK_Msk |*/
			/*USB_OTG_DIEPCTL_EPENA_Msk |*/ USB_OTG_DIEPCTL_USBAEP_Msk;
	epinval = *EP_IN(USB, epnum);
	printf(ESC_DEBUG "%lu\tusb_hw: " ESC_WRITE "EP %lu IN" ESC_DEBUG " after 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n",
	       systick_cnt(), epnum, epinval.DIEPCTL, epinval.DIEPINT, epinval.DIEPTSIZ, epin->DIEPDMA);
#endif
	epin->DIEPCTL = ((epin->DIEPCTL) & (USB_OTG_DIEPCTL_TXFNUM_Msk | USB_OTG_DIEPCTL_MPSIZ_Msk |
			USB_OTG_DIEPCTL_EPTYP_Msk)) | USB_OTG_DIEPCTL_CNAK_Msk |
			USB_OTG_DIEPCTL_EPENA_Msk | USB_OTG_DIEPCTL_USBAEP_Msk;
#if DEBUG >= 6
	printf(ESC_DEBUG "%lu\tusb_hw: " ESC_WRITE "EP %lu IN" ESC_DEBUG " transferred %lu bytes\n",
	       systick_cnt(), epnum, size);
#endif
}

void usb_hw_ep_in_nak(uint32_t epnum)
{
	USB_OTG_INEndpointTypeDef *epin = EP_IN(USB, epnum);
#if 0	// Apparently this is normal after USB reset
#if DEBUG
	if (epin->DIEPCTL & USB_OTG_DIEPCTL_EPENA_Msk)
		USB_ERROR();
#endif
#endif
	epin->DIEPCTL = ((epin->DIEPCTL) & (USB_OTG_DIEPCTL_TXFNUM_Msk | USB_OTG_DIEPCTL_MPSIZ_Msk |
			USB_OTG_DIEPCTL_EPTYP_Msk)) | USB_OTG_DIEPCTL_SNAK_Msk | USB_OTG_DIEPCTL_USBAEP_Msk;
#if DEBUG >= 6
	printf(ESC_DEBUG "%lu\tusb_hw: " ESC_WRITE "EP %lu IN" ESC_DEBUG " NAK\n", systick_cnt(), epnum);
#endif
}

void OTG_HS_IRQHandler()
{
	USB_OTG_GlobalTypeDef *usb = USB;
	USB_OTG_DeviceTypeDef *dev = DEV(USB);
	uint32_t ists = USB->GINTSTS;
	uint32_t dsts = dev->DSTS;
	uint32_t daint = dev->DAINT;
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

	// OUT endpoint events
	if (ists & USB_OTG_GINTSTS_OEPINT_Msk) {
		uint32_t mask = FIELD(daint, USB_OTG_DAINTMSK_OEPM);
		for (uint32_t i = 0; mask; i++, mask >>= 1) {
			if (!(mask & 1))
				continue;
			USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(USB, i);
			uint32_t epint = ep->DOEPINT;
			uint32_t epsize = ep->DOEPTSIZ;
			uint32_t processed = 0;
			void *epdma = (void *)ep->DOEPDMA;
			uint32_t xfrsize = ep_out.ep[i].xfrsize - FIELD(epsize, USB_OTG_DOEPTSIZ_XFRSIZ);

			if (epint & USB_OTG_DOEPINT_STUP_Msk) {
				ep->DOEPINT = USB_OTG_DOEPINT_STUP_Msk;
				if (ep_out.ep[i].func.stup)
					(*ep_out.ep[i].func.stup)(epdma, xfrsize);
				processed = 1;
			}
			if (epint & USB_OTG_DOEPINT_OTEPSPR_Msk) {
				ep->DOEPINT = USB_OTG_DOEPINT_OTEPSPR_Msk;
				if (ep_out.ep[i].func.stsp)
					(*ep_out.ep[i].func.stsp)(epdma, xfrsize);
				processed = 1;
			}
			if (epint & USB_OTG_DOEPINT_XFRC_Msk) {
				ep->DOEPINT = USB_OTG_DOEPINT_XFRC_Msk;
#if DEBUG >= 6
				dbgprintf(ESC_DEBUG "%lu\tusb_hw: " ESC_READ "EP %lu OUT"
					  ESC_DEBUG " transferred %lu bytes\n",
					  systick_cnt(), i, xfrsize);
#endif
				if (ep_out.ep[i].func.xfrc)
					(*ep_out.ep[i].func.xfrc)(epdma, xfrsize);
				processed = 1;
			}
			if (!processed)
				USB_TODO();
		}
		processed = 1;
	}

	// IN endpoint events
	if (ists & USB_OTG_GINTSTS_IEPINT_Msk) {
		uint32_t mask = FIELD(daint, USB_OTG_DAINTMSK_IEPM);
		for (uint32_t i = 0; mask; i++, mask >>= 1) {
			if (!(mask & 1))
				continue;
			USB_OTG_INEndpointTypeDef *ep = EP_IN(USB, i);
			uint32_t epint = ep->DIEPINT;
			uint32_t processed = 0;
#if DEBUG >= 6
			printf(ESC_DEBUG "%lu\tusb_hw: " ESC_WRITE "EP %lu IN"
			       ESC_DEBUG " interrupt 0x%08lx\n", systick_cnt(), i, epint);
#endif

			if (epint & USB_OTG_DIEPINT_XFRC_Msk) {
				ep->DIEPINT = USB_OTG_DIEPINT_XFRC_Msk;
				if (ep_in.ep[i].func.xfrc)
					(*ep_in.ep[i].func.xfrc)(ep_in.ep[i].p, ep_in.ep[i].xfrsize);
				processed = 1;
			}
			if (epint & USB_OTG_DIEPINT_TOC_Msk) {
				ep->DIEPINT = USB_OTG_DIEPINT_TOC_Msk;
				USB_TODO();
				processed = 1;
			}
			if (!processed)
				USB_TODO();
		}
		processed = 1;
	}

#if 0
	// OTG event
	if (ists & USB_OTG_GINTSTS_OTGINT_Msk) {
	}
#endif

	// USB mode mismatch event
	if (ists & USB_OTG_GINTSTS_MMIS_Msk) {
		// Disconnect and panic
		USB_ERROR();
	}

	// USB suspend
	if (ists & (USB_OTG_GINTSTS_USBSUSP_Msk)) {
		LIST_ITERATE(usb_susp, usb_basic_handler_t, p) (*p)();
		USB->GINTSTS = USB_OTG_GINTSTS_USBSUSP_Msk;
		// Ignored for now
		processed = 1;
	}

	// USB reset
	if (ists & USB_OTG_GINTSTS_USBRST_Msk) {
#if DEBUG
		printf(ESC_DEBUG "%lu\tusb_hw: USB reset\n", systick_cnt());
#endif
		LIST_ITERATE(usb_reset, usb_basic_handler_t, p) (*p)();
		USB->GINTSTS = USB_OTG_GINTSTS_USBRST_Msk;
		processed = 1;
	}

	// USB emulation done
	if (ists & USB_OTG_GINTSTS_ENUMDNE_Msk) {
#if DEBUG >= 5
		if (LIST_SIZE(usb_enum) == 0)
			USB_TODO();
#endif
		uint32_t spd = DSTS_ENUM_SPD(dev->DSTS);
#if DEBUG
		printf(ESC_DEBUG "%lu\tusb_hw: USB enumerate %lu\n", systick_cnt(), spd);
#endif
		LIST_ITERATE(usb_enum, usb_enum_handler_t, p) (*p)(spd);
		USB->GINTSTS = USB_OTG_GINTSTS_ENUMDNE_Msk;
		processed = 1;
	}

	// Unimplemented interrupt
	if (!processed) {
		printf(ESC_WARNING "%lu\tusb_hw: Unknown interrupt: 0x%08lx\n", systick_cnt(), ists);
		USB_ERROR();
	}
}
