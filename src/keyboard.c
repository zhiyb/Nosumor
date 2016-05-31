#include <stdint.h>
#include "clocks.h"
#include "keyboard.h"
#include "usb.h"
#include "usb_desc.h"
#include "usb_class.h"
#include "usart1.h"
#include "debug.h"

#define KEYCODE_LEFT	0x1d
#define KEYCODE_RIGHT	0x1b

#define TIM_PSC		32
#define TIM_CLK		(APB1_TIM_CLK / TIM_PSC)

#define KEY_ITVL	5

#define EXTIx(gp, pin)	(((gp) - GPIOA) / (GPIOB - GPIOA)) << (((pin) & 3) << 2)
#define KEYS		(BV(KEY_LEFT) | BV(KEY_RIGHT) | BV(KEY_1) | BV(KEY_2) | BV(KEY_3))

static union {
	uint32_t mem[5];
} hid USBRAM;

STATIC_ASSERT(sizeof(hid) / 2 <= EP1_SIZE, "EP1_SIZE too small");

static const uint16_t keysBV[] = {BV(KEY_LEFT), BV(KEY_RIGHT), BV(KEY_1), BV(KEY_2), BV(KEY_3)};

static struct {
	volatile uint16_t keys;
	volatile uint16_t valid;
	uint16_t keysIRQ;
	uint16_t itvl;	// Debouncing interval
	uint16_t compare[5];
	uint16_t compareDMA[8];
	uint16_t *ptrDMA;
} status;

STATIC_ASSERT(ARRAY_SIZE(status.compareDMA) == 8, "Need to be 8");

static void updateReport()
{
	uint16_t keys = status.keys;
	hid.mem[1] = !(keys & BV(KEY_LEFT)) ? KEYCODE_LEFT << 8 : 0;
	hid.mem[2] = !(keys & BV(KEY_RIGHT)) ? KEYCODE_RIGHT : 0;
}

static inline void initTimer()
{
	// Setup debouncing timer (TIM2)
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	// Continuous, counting up until overflow
	memset(TIM2, 0, sizeof(*TIM2));
	TIM2->PSC = TIM_PSC - 1;
	TIM2->ARR = 0xffff;

	// Using DMA to transfer TIM2 CCR4
	TIM2->DCR = ((uint32_t)&TIM2->CCR4 - (uint32_t)TIM2) >> 2;
	// Using DMA1 channel 7 (trigger on TIM2 CH4)
	// 16 bits, circular mode, high priority
	DMA1_Channel7->CCR = DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_PL_1 |
			DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_DIR;
	DMA1_Channel7->CNDTR = ARRAY_SIZE(status.compareDMA);
	DMA1_Channel7->CPAR = (uint32_t)&TIM2->DMAR;
	DMA1_Channel7->CMAR = (uint32_t)status.compareDMA;
	DMA1_Channel7->CCR |= DMA_CCR_EN;

	// Enable timer interrupt
	TIM2->CCER = TIM_CCER_CC4E;
	TIM2->DIER = TIM_DIER_CC4IE;
	uint32_t prioritygroup = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(TIM2_IRQn, NVIC_EncodePriority(prioritygroup, 1, 1));
	NVIC_EnableIRQ(TIM2_IRQn);

	// Enable timer
	TIM2->CR1 |= TIM_CR1_CEN;
}

static inline void initGPIO()
{
	// Setup GPIOs
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

	// 00, 01, 0: General purpose output push-pull, 10MHz, reset
	HL(LED_GPIO->CRL, LED_LEFT) &= ~(0x0c << (MOD8(LED_LEFT) << 2) |
					 0x0c << (MOD8(LED_RIGHT) << 2) |
					 0x0c << (MOD8(LED_RED) << 2) |
					 0x0c << (MOD8(LED_GREEN) << 2) |
					 0x0c << (MOD8(LED_BLUE) << 2));
	HL(LED_GPIO->CRL, LED_LEFT) |= (0x01 << (MOD8(LED_LEFT) << 2) |
					0x01 << (MOD8(LED_RIGHT) << 2) |
					0x01 << (MOD8(LED_RED) << 2) |
					0x01 << (MOD8(LED_GREEN) << 2) |
					0x01 << (MOD8(LED_BLUE) << 2));
	LED_GPIO->BRR = (BV(LED_LEFT) | BV(LED_RIGHT) |
			 BV(LED_RED) | BV(LED_GREEN) | BV(LED_BLUE));

	// 10, 00, 1: Input with pull-up
	HL(KEY_GPIO->CRL, KEY_LEFT) &= ~(0x07 << (MOD8(KEY_LEFT) << 2) |
					 0x07 << (MOD8(KEY_RIGHT) << 2) |
					 0x07 << (MOD8(KEY_1) << 2) |
					 0x07 << (MOD8(KEY_2) << 2) |
					 0x07 << (MOD8(KEY_3) << 2));
	HL(KEY_GPIO->CRL, KEY_LEFT) |= (0x08 << (MOD8(KEY_LEFT) << 2) |
					0x08 << (MOD8(KEY_RIGHT) << 2) |
					0x08 << (MOD8(KEY_1) << 2) |
					0x08 << (MOD8(KEY_2) << 2) |
					0x08 << (MOD8(KEY_3) << 2));
	KEY_GPIO->BSRR = KEYS;

	// Setup external interrupts
	AFIO->EXTICR[KEY_LEFT / 4] |= EXTIx(KEY_GPIO, KEY_LEFT);
	AFIO->EXTICR[KEY_RIGHT / 4] |= EXTIx(KEY_GPIO, KEY_RIGHT);
	AFIO->EXTICR[KEY_1 / 4] |= EXTIx(KEY_GPIO, KEY_1);
	AFIO->EXTICR[KEY_2 / 4] |= EXTIx(KEY_GPIO, KEY_2);
	AFIO->EXTICR[KEY_3 / 4] |= EXTIx(KEY_GPIO, KEY_3);

	EXTI->RTSR |= KEYS;
	EXTI->FTSR |= KEYS;
	EXTI->PR = KEYS;
	EXTI->IMR |= KEYS;

	// Enable external interrupts
	status.keys = status.keysIRQ = KEY_GPIO->IDR & KEYS;
	status.valid = KEYS;
	updateReport();
	uint32_t prioritygroup = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(EXTI9_5_IRQn, NVIC_EncodePriority(prioritygroup, 1, 0));
	NVIC_SetPriority(EXTI15_10_IRQn, NVIC_EncodePriority(prioritygroup, 1, 0));
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void initKeyboard()
{
	// Initialise status variables
	status.itvl = TIM_CLK * KEY_ITVL / 1000;
	memset(&hid, 0, sizeof(hid));
	hid.mem[0] = HID_KEYBOARD;
	eptable[1][EP_TX].addr = USB_LOCAL_ADDR(&hid);
	eptable[1][EP_TX].count = 9;

	initTimer();
	initGPIO();
}

uint32_t readKey(uint16_t key)
{
	return !(status.keys & BV(key));
}

static void keyboardIRQ()
{
	NVIC_DisableIRQ(EXTI9_5_IRQn);
	NVIC_DisableIRQ(EXTI15_10_IRQn);
	status.keysIRQ = KEY_GPIO->IDR;
	uint16_t pr = EXTI->PR;
	EXTI->PR = pr;
	uint16_t compare = TIM2->CNT;

	status.keysIRQ &= KEYS;
	pr &= status.valid;

	if (pr) {
		compare += status.itvl;
		status.keys ^= pr;
		// Update and send HID report
		updateReport();
		usbValid(1, EP_TX);
		// Add to debouncing timer
		typeof(&keysBV[0]) key = keysBV;
		uint32_t i;
		for (i = 0; i != ARRAY_SIZE(keysBV); key++, i++)
			if (pr & *key)
				status.compare[i] = compare;
		if (status.valid == KEYS) {
			TIM2->CCR4 = compare;
			uint32_t cnt = (ARRAY_SIZE(status.compareDMA) - DMA1_Channel7->CNDTR - 1) & 7;
			status.ptrDMA = status.compareDMA + cnt;
			*status.ptrDMA = compare;
			TIM2->DIER |= TIM_DIER_CC4DE;
		} else if (compare != TIM2->CCR4 && compare != *status.ptrDMA) {
			uint32_t offset = status.ptrDMA - status.compareDMA;
			status.ptrDMA = status.compareDMA + ((offset + 1) & 7);
			*status.ptrDMA = compare;

			uint32_t cnt = (ARRAY_SIZE(status.compareDMA) - DMA1_Channel7->CNDTR - 1) & 7;
			uint16_t *ptr = status.compareDMA + cnt;
			if (ptr == status.ptrDMA)
				TIM2->CCR4 = compare;
		}
		status.valid &= ~pr;
#ifdef DEBUG
		key = keysBV;
		for (i = 0; i != ARRAY_SIZE(keysBV); key++, i++)
			if (pr & *key)
				usart1WriteChar(i + (*key & status.keys ? 'a' : 'A'));
#endif
	}

	NVIC_EnableIRQ(EXTI9_5_IRQn);
	NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void EXTI9_5_IRQHandler() __attribute__((weak, alias("keyboardIRQ")));
void EXTI15_10_IRQHandler() __attribute__((weak, alias("keyboardIRQ")));

void TIM2_IRQHandler()
{
	TIM2->SR = 0;
	uint16_t cnt = TIM2->CNT;
	if (status.valid == KEYS)
		return;

	typeof(&keysBV[0]) key = keysBV;
	uint16_t valid = 0;
	uint32_t i;
	for (i = 0; i != ARRAY_SIZE(status.compare); key++, i++)
		if (!(status.valid & *key)) {
			uint16_t diff = cnt - status.compare[i];
			if (!(diff & 0x8000)) {	// diff > 0
				valid |= *key;
#ifdef DEBUG
				if (diff & 0xfffe) {	// diff > 1
					usart1WriteChar('<');
					usart1DumpHex(diff);
					usart1WriteChar('>');
				}
#endif
			}
		}
#ifdef DEBUG
	if (!valid)
		usart1WriteChar('-');
#endif
	NVIC_DisableIRQ(EXTI9_5_IRQn);
	NVIC_DisableIRQ(EXTI15_10_IRQn);
	status.valid |= valid;
	uint16_t keys = KEY_GPIO->IDR & KEYS;
	EXTI->SWIER = keys ^ status.keys;
	if (status.valid == KEYS)
		TIM2->DIER &= ~TIM_DIER_CC4DE;
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	NVIC_EnableIRQ(EXTI15_10_IRQn);
}
