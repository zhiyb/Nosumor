#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <device.h>
#include <module.h>
#include <irq.h>
#include <debug.h>
#include <systick.h>
#include "keyboard.h"

// Number of keys
#define KEYBOARD_KEYS		5
// Debouncing time (SysTick counts)
#define KEYBOARD_DEBOUNCING	5

#if HWVER >= 0x0100
// KEY_12:	K1(PB4), K2(PB2)
// KEY_345:	K3(PC13), K4(PC14), K5(PC15)
#define KEYBOARD_MASK_1	(1ul << 4)
#else
// KEY_12:	K1(PA6), K2(PB2)
// KEY_345:	K3(PC13), K4(PC14), K5(PC15)
#define KEYBOARD_MASK_1	(1ul << 6)
#endif
#define KEYBOARD_MASK_2	(1ul << 2)
#define KEYBOARD_MASK_3	(1ul << 13)
#define KEYBOARD_MASK_4	(1ul << 14)
#define KEYBOARD_MASK_5	(1ul << 15)

#if KEYBOARD_KEYS != 5
#error Unsupported keyboard key count
#endif

#define KEYBOARD_Msk	(KEYBOARD_MASK_1 | KEYBOARD_MASK_2 | \
	KEYBOARD_MASK_3 | KEYBOARD_MASK_4 | KEYBOARD_MASK_5)

LIST(keyboard, keyboard_callback_t);

static const struct {
	uint32_t keys;
	uint32_t masks[KEYBOARD_KEYS];
	const char *names[KEYBOARD_KEYS];
} info = {
	KEYBOARD_KEYS, {
		KEYBOARD_MASK_1, KEYBOARD_MASK_2,
		KEYBOARD_MASK_3, KEYBOARD_MASK_4, KEYBOARD_MASK_5,
	}, {
		"Left", "Right", "K1", "K2", "K3",
	}
};

#if 0
uint8_t keycodes[KEYBOARD_KEYS] = {
	// z,    x,    c,    ~,  ESC
	0x1d, 0x1b, 0x06, 0x35, 0x29,
};
#endif
static volatile uint32_t status, debouncing, timeout[KEYBOARD_KEYS];

static uint32_t keyboard_gpio_status();

static void keyboard_init()
{
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
	// Mask interrupts
	EXTI->IMR &= ~KEYBOARD_Msk;

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
}

static void keyboard_start()
{
	// Clear interrupt flags
	EXTI->PR = KEYBOARD_Msk;
	// Update keyboard status
	status = keyboard_gpio_status();
	debouncing = 0;
	// Unmask interrupts
	EXTI->IMR |= KEYBOARD_Msk;
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

static void keyboard_update(uint32_t status)
{
	LIST_ITERATE(keyboard, const keyboard_callback_t *p, p)
		(*p)(status);
#if 0
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
	if (api_config_data.keyboard)
		usb_hid_update(hid.keyboard);
	// Update USB HID mouse report
	if (!hid.mouse)
		goto js;
	if (!api_config_data.mouse)
		goto js;
	hid.mouse->report.payload[0] = mask >> 3;
	usb_hid_update(hid.mouse);
js:	// Update joystick report
	if (!hid.joystick)
		return;
	if (!api_config_data.joystick)
		return;
	hid.joystick->report.payload[12] = mask >> 3;
	usb_hid_update(hid.joystick);
#endif
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
	uint32_t to = (uint32_t)MODULE_MSG(module_init, "tick.get", 0);
	const uint32_t *pm = info.masks;
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
	const uint32_t *pm = info.masks;
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
		dbgprintf(ESC_ERROR "[KEY] Too short\n");
}

// Register debouncing tick
SYSTICK_CALLBACK(&keyboard_tick);

static void *handler(void *inst, uint32_t msg, void *data)
{
	UNUSED(inst);
	UNUSED(data);
	if (msg == HASH("status")) {
		return (void *)status;
	} else if (msg == HASH("init")) {
		keyboard_init();
		return 0;
	} else if (msg == HASH("start")) {
		keyboard_start();
		return 0;
	} else if (msg == HASH("info")) {
		return (void *)&info;
	}
	return 0;
}

MODULE(keyboard, 0, 0, handler);
