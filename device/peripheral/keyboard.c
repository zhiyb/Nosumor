#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stm32f7xx.h>
#include <irq.h>
#include <macros.h>
#include <debug.h>
#include <system/systick.h>
#include <usb/hid/usb_hid.h>
#include "keyboard.h"

// Debouncing time (SysTick counts)
#define KEYBOARD_DEBOUNCING	5

#if KEYBOARD_KEYS != 5
#error Unsupported keyboard key count
#endif

#define KEYBOARD_Msk	(KEYBOARD_MASK_1 | KEYBOARD_MASK_2 | \
	KEYBOARD_MASK_3 | KEYBOARD_MASK_4 | KEYBOARD_MASK_5)

const uint32_t keyboard_masks[KEYBOARD_KEYS] = {
	KEYBOARD_MASK_1, KEYBOARD_MASK_2, KEYBOARD_MASK_3,
	KEYBOARD_MASK_4, KEYBOARD_MASK_5,
};

static const char keyboard_names[KEYBOARD_KEYS][8] = {
	"Left", "Right", "K1", "K2", "K3",
};

uint8_t keycodes[KEYBOARD_KEYS] = {
	// z,    x,    c,    ~,  ESC
	0x1d, 0x1b, 0x06, 0x35, 0x29,
};

static struct {
	usb_hid_if_t *keyboard, *joystick;
} hid;
static volatile uint32_t status, debouncing, timeout[KEYBOARD_KEYS];

static uint32_t keyboard_gpio_status();
static void keyboard_tick(uint32_t tick);

const char *keyboard_name(unsigned int btn)
{
	if (btn < ASIZE(keyboard_names))
		return keyboard_names[btn];
	printf(ESC_ERROR "[KEY] Invalid button: %u\n", btn);
	return 0;
}

void keyboard_init(usb_hid_if_t *hid_keyboard, usb_hid_if_t *hid_joystick)
{
	hid.keyboard = hid_keyboard;
	hid.joystick = hid_joystick;

	// Initialise GPIOs
#if HWVER >= 0x0100
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
	// PB4
	GPIO_MODER(GPIOB, 4, 0b00);	// 00: Input mode
	GPIO_PUPDR(GPIOB, 4, GPIO_PUPDR_UP);
#else
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
	// PA6
	GPIO_MODER(GPIOA, 6, 0b00);	// 00: Input mode
	GPIO_PUPDR(GPIOA, 6, GPIO_PUPDR_UP);
#endif
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
#if HWVER >= 0x0100
	exticr_exti(4, EXTICR_EXTI_PB);
#else
	exticr_exti(6, EXTICR_EXTI_PA);
#endif
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
	NVIC_EnableIRQ(EXTI2_IRQn);
#if HWVER >= 0x0100
	NVIC_SetPriority(EXTI4_IRQn,
			 NVIC_EncodePriority(pg, NVIC_PRIORITY_KEYBOARD, 1));
	NVIC_EnableIRQ(EXTI4_IRQn);
#else
	NVIC_SetPriority(EXTI9_5_IRQn,
			 NVIC_EncodePriority(pg, NVIC_PRIORITY_KEYBOARD, 1));
	NVIC_EnableIRQ(EXTI9_5_IRQn);
#endif
	NVIC_SetPriority(EXTI15_10_IRQn,
			 NVIC_EncodePriority(pg, NVIC_PRIORITY_KEYBOARD, 2));
	NVIC_EnableIRQ(EXTI15_10_IRQn);

	// Register debouncing tick
	systick_register_handler(&keyboard_tick);
}

static uint32_t keyboard_gpio_status()
{
	uint32_t stat = ~KEYBOARD_Msk;
#if HWVER >= 0x0100
	stat |= GPIOB->IDR & GPIO_IDR_IDR_4;
#else
	stat |= GPIOA->IDR & GPIO_IDR_IDR_6;
#endif
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

void keyboard_update(uint32_t status)
{
	// Clear keyboard status
	memset(hid.keyboard->report.payload, 0, hid.keyboard->size - 1u);
	// Modifier keys (0), reserved (1), keycodes (2*)
	uint8_t *p = &hid.keyboard->report.payload[2];
	// Update report
	const uint32_t *pm = keyboard_masks;
	uint8_t mask = 0;
	for (uint32_t i = 0; i != KEYBOARD_KEYS; i++) {
		mask >>= 1;
		if (status & *pm++) {
			*p++ = keycodes[i];
			mask |= 0x80;
		}
	}
	// Send report
	usb_hid_update(hid.keyboard);
	// Update joystick report
	if (!hid.joystick)
		return;
	hid.joystick->report.payload[12] = mask >> 3;
	usb_hid_update(hid.joystick);
}

static void keyboard_irq()
{
	uint32_t irq = EXTI->PR & KEYBOARD_Msk;
	if (!irq)
		return;
	// Disable interrupt triggers for debouncing
	EXTI->RTSR &= ~irq;
	EXTI->FTSR &= ~irq;
	EXTI->PR = irq;

	// IRQ should be different from here even if reentrant
	status ^= irq;
	keyboard_update(status);
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
#if HWVER >= 0x0100
void EXTI4_IRQHandler() __attribute__((alias("keyboard_irq")));
#else
void EXTI9_5_IRQHandler() __attribute__((alias("keyboard_irq")));
#endif
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
	uint32_t err = (status ^ stat) & mask;
	EXTI->SWIER = err;
	debouncing &= ~mask;
	__enable_irq();
	if (err)
		dbgprintf(ESC_ERROR "[KEY] Tick mismatch\n");
}

uint8_t keyboard_keycode(unsigned int btn)
{
	if (btn < ASIZE(keycodes))
		return keycodes[btn];
	printf(ESC_ERROR "[KEY] Invalid button: %u\n", btn);
	return 0;
}

void keyboard_keycode_set(unsigned int btn, uint8_t code)
{
	if (btn < ASIZE(keycodes))
		keycodes[btn] = code;
	else
		printf(ESC_ERROR "[KEY] Invalid button: %u\n", btn);
}
