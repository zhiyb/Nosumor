#include <stdint.h>
#include <stdio.h>
#include <stm32f7xx.h>
#include "../../irq.h"
#include "../../systick.h"
#include "../../macros.h"
#include "../../debug.h"
#include "keyboard.h"

// KEY_12:	PA6(K1), PB2(K2)
// KEY_345:	PC13(K3), PC14(K4), PC15(K5)

// Debouncing time (SysTick counts)
#define KEYBOARD_DEBOUNCING	5

#define KEYBOARD_Msk	((1ul << 2) | (1ul << 6) | (1ul << 13) | (1ul << 14) | (1ul << 15))

const uint32_t keyboard_masks[KEYBOARD_KEYS] = {
	1ul << 2, 1ul << 6, 1ul << 13, 1ul << 14, 1ul << 15
};

static volatile uint32_t status, debouncing, timeout[KEYBOARD_KEYS];

static uint32_t keyboard_gpio_status();
static void keyboard_tick(uint32_t tick);

void keyboard_init()
{
	// Initialise GPIOs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
	// PA6
	GPIO_MODER(GPIOA, 6, 0b00);	// 00: Input mode
	GPIO_PUPDR(GPIOA, 6, GPIO_PUPDR_UP);
	// PB2
	GPIO_MODER(GPIOB, 2, 0b00);
	GPIO_PUPDR(GPIOB, 2, GPIO_PUPDR_UP);
	// PC13, PC14, PC15
	GPIO_MODER(GPIOC, 13, 0b00);
	GPIO_MODER(GPIOC, 14, 0b00);
	GPIO_MODER(GPIOC, 15, 0b00);
	GPIO_PUPDR(GPIOC, 13, GPIO_PUPDR_UP);
	GPIO_PUPDR(GPIOC, 14, GPIO_PUPDR_UP);
	GPIO_PUPDR(GPIOC, 15, GPIO_PUPDR_UP);

	// Setup interrupts
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN_Msk;
	exticr_exti(6, EXTICR_EXTI_PA);
	exticr_exti(2, EXTICR_EXTI_PB);
	exticr_exti(13, EXTICR_EXTI_PC);
	exticr_exti(14, EXTICR_EXTI_PC);
	exticr_exti(15, EXTICR_EXTI_PC);
	// Rising and falling edge trigger
	EXTI->RTSR |= KEYBOARD_Msk;
	EXTI->FTSR |= KEYBOARD_Msk;
	// Clear interrupts
	EXTI->PR = KEYBOARD_Msk;
	// Unmask interrupts
	EXTI->IMR |= KEYBOARD_Msk;
	status = keyboard_gpio_status();
	debouncing = 0ul;

	// Enable NVIC interrupts
	uint32_t pg = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(EXTI2_IRQn,
			 NVIC_EncodePriority(pg, NVIC_PRIORITY_KEYBOARD, 0));
	NVIC_SetPriority(EXTI9_5_IRQn,
			 NVIC_EncodePriority(pg, NVIC_PRIORITY_KEYBOARD, 1));
	NVIC_SetPriority(EXTI15_10_IRQn,
			 NVIC_EncodePriority(pg, NVIC_PRIORITY_KEYBOARD, 2));
	NVIC_EnableIRQ(EXTI2_IRQn);
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	NVIC_EnableIRQ(EXTI15_10_IRQn);

	// Register debouncing tick
	systick_register_handler(&keyboard_tick);
}

static uint32_t keyboard_gpio_status()
{
	uint32_t stat = ~KEYBOARD_Msk;
	stat |= GPIOA->IDR & GPIO_IDR_IDR_6;
	stat |= GPIOB->IDR & GPIO_IDR_IDR_2;
	stat |= GPIOC->IDR & GPIO_IDR_IDR_13;
	stat |= GPIOC->IDR & GPIO_IDR_IDR_14;
	stat |= GPIOC->IDR & GPIO_IDR_IDR_15;
	return ~stat;
}

uint32_t keyboard_status()
{
	return status;
}

static void keyboard_irq()
{
	// Mutual exclusion interrupt
	__disable_irq();
	uint32_t irq = EXTI->PR & KEYBOARD_Msk;
	if (!irq) {
		__enable_irq();
		return;
	}
	NVIC_DisableIRQ(EXTI2_IRQn);
	NVIC_DisableIRQ(EXTI9_5_IRQn);
	NVIC_DisableIRQ(EXTI15_10_IRQn);
	__enable_irq();
	// Disable interrupt triggers for debouncing
	EXTI->RTSR &= ~irq;
	EXTI->FTSR &= ~irq;
	EXTI->PR = irq;

	// IRQ should be different from here even if reentrant
	status ^= irq;
	usb_keyboard_update(status);
	// Critical section end
	NVIC_EnableIRQ(EXTI2_IRQn);
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	NVIC_EnableIRQ(EXTI15_10_IRQn);
	// Debouncing setup
	debouncing |= irq;
	// Timeout value for debouncing
	uint32_t to = systick_cnt();
	const uint32_t *pm = keyboard_masks;
	for (uint32_t i = 0; i != KEYBOARD_KEYS; i++, pm++)
		if (irq & *pm)
			timeout[i] = to;
}

void EXTI2_IRQHandler() __attribute__((alias("keyboard_irq")));
void EXTI9_5_IRQHandler() __attribute__((alias("keyboard_irq")));
void EXTI15_10_IRQHandler() __attribute__((alias("keyboard_irq")));

static void keyboard_tick(uint32_t tick)
{
	uint32_t db = debouncing;
	if (!db)
		return;
	uint32_t stat = keyboard_gpio_status(), mask = 0;
	const uint32_t *pm = keyboard_masks;
	for (uint32_t i = 0; i != KEYBOARD_KEYS; i++, pm++)
		if ((db & *pm) && tick - timeout[i] > KEYBOARD_DEBOUNCING)
			mask |= *pm;
	if (!mask)
		return;
	__disable_irq();
	EXTI->RTSR |= mask;
	EXTI->FTSR |= mask;
	uint32_t err = (status & mask) ^ (stat & mask);
	EXTI->SWIER = err;
	if (err)
		dbgbkpt();
	debouncing &= ~mask;
	__enable_irq();
}
