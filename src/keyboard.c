#include <stdint.h>
#include "keyboard.h"
#include "usart1.h"
#include "debug.h"

#define EXTIx(gp, pin)	(((gp) - GPIOA) / (GPIOB - GPIOA)) << (((pin) & 3) << 2)
#define KEYS		(BV(KEY_LEFT) | BV(KEY_RIGHT) | BV(KEY_1) | BV(KEY_2) | BV(KEY_3))

void initKeyboard()
{
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

	AFIO->EXTICR[KEY_LEFT / 4] |= EXTIx(KEY_GPIO, KEY_LEFT);
	AFIO->EXTICR[KEY_RIGHT / 4] |= EXTIx(KEY_GPIO, KEY_RIGHT);
	AFIO->EXTICR[KEY_1 / 4] |= EXTIx(KEY_GPIO, KEY_1);
	AFIO->EXTICR[KEY_2 / 4] |= EXTIx(KEY_GPIO, KEY_2);
	AFIO->EXTICR[KEY_3 / 4] |= EXTIx(KEY_GPIO, KEY_3);

	EXTI->RTSR |= KEYS;
	EXTI->FTSR |= KEYS;
	EXTI->PR = KEYS;
	EXTI->IMR |= KEYS;

	// Setup external interrupts
	uint32_t prioritygroup = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(EXTI9_5_IRQn, NVIC_EncodePriority(prioritygroup, 1, 0));
	NVIC_SetPriority(EXTI15_10_IRQn, NVIC_EncodePriority(prioritygroup, 1, 0));
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static void keyboardIRQ()
{
	toggleLED(LED_LEFT);
	uint16_t pr = EXTI->PR;
	if (pr & KEYS)
		usart1WriteChar('E');
	EXTI->PR = pr;
}

void EXTI9_5_IRQHandler() __attribute__((weak, alias("keyboardIRQ")));
void EXTI15_10_IRQHandler() __attribute__((weak, alias("keyboardIRQ")));
