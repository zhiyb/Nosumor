#include <device.h>
#include <macros.h>
#include <debug.h>
#include <system/irq.h>
#include <system/systick.h>
#include <usb/hid/keyboard/usb_hid_keyboard.h>
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

const char keyboard_names[KEYBOARD_KEYS][8] = {
	"Left", "Right", "K1", "K2", "K3",
};

uint8_t keyboard_keycodes[KEYBOARD_KEYS] = {
	// z,    x,    c,    ~,  ESC
	0x1d, 0x1b, 0x06, 0x35, 0x29,
};

static struct {
	volatile uint32_t status;
	volatile uint32_t debouncing;
	volatile uint32_t glitch;
	volatile uint32_t timeout[KEYBOARD_KEYS];
	struct {
		volatile uint32_t locked;
		volatile uint32_t pending;
	} hid;
} data;

static uint32_t keyboard_gpio_status();
static void keyboard_tick(uint32_t tick);
static inline void keyboard_hid_update(uint32_t status);

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
	// Unmask interrupts
	EXTI->IMR |= KEYBOARD_Msk;
	data.status = keyboard_gpio_status();
	data.debouncing = 0ul;

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

INIT_HANDLER(&keyboard_init);

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
	return data.status;
}

static void keyboard_irq()
{
	__disable_irq();
	uint32_t irq = EXTI->PR & KEYBOARD_Msk;
	uint32_t tout = systick_cnt();
	if (!irq) {
		__enable_irq();
		return;
	}
	// Disable interrupt triggers for debouncing
	EXTI->RTSR &= ~irq;
	EXTI->FTSR &= ~irq;
	EXTI->PR = irq;
	data.status ^= irq;
	uint32_t locked = data.hid.locked;
	if (locked)
		data.hid.pending = 1;
	else
		data.hid.locked = 1;
	__enable_irq();

	// Send HID report if acquired HID lock or pending
	if (!locked) {
		for (uint32_t pending = 1; pending;) {
			keyboard_hid_update(data.status);
			__disable_irq();
			pending = data.hid.pending;
			if (pending)
				data.hid.pending = 0;
			else
				data.hid.locked = 0;
			__enable_irq();
		}
	}
	// Timeout values for debouncing
	for (uint32_t i = 0; i != KEYBOARD_KEYS; i++)
		if (irq & keyboard_masks[i])
			data.timeout[i] = tout;

	__disable_irq();
	// Debouncing start
	data.debouncing |= irq;
	__enable_irq();
	// TODO keyboard_update(status);
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
	uint32_t db = data.debouncing;
	if (!db)
		return;
	uint32_t stat = keyboard_gpio_status(), mask = 0, tout = tick - KEYBOARD_DEBOUNCING;
	for (uint32_t i = 0; i != KEYBOARD_KEYS; i++)
		if ((db & keyboard_masks[i]) && (int32_t)(tout - data.timeout[i]) >= 0)
			mask |= keyboard_masks[i];
	if (!mask)
		return;

	__disable_irq();
	EXTI->RTSR |= mask;
	EXTI->FTSR |= mask;
	data.debouncing &= ~mask;
	// Trigger status update for mismatched keys
	uint32_t err = (data.status ^ stat) & mask;
	EXTI->SWIER = err;
	__enable_irq();
	data.glitch |= err;
}

// Register debouncing tick as systick handler
SYSTICK_HANDLER(&keyboard_tick);

static inline void keyboard_hid_update(uint32_t status)
{
	usb_hid_keyboard_report_t report = {0};
	uint32_t key = 0;
	for (uint32_t i = 0; i != KEYBOARD_KEYS; i++)
		if (status & keyboard_masks[i])
			report.key[key++] = keyboard_keycodes[i];
	usb_hid_keyboard_report_update(&report);
}

#if DEBUG
static void debug_keyboard()
{
#if DEBUG >= 5
	static uint32_t _status = 0;
	uint32_t status = keyboard_status();
	if (status != _status) {
		printf(ESC_MSG "%lu\tkeyboard:", systick_cnt());
		for (uint32_t i = 0; i < KEYBOARD_KEYS; i++)
			printf(" %s%s", (status & keyboard_masks[i]) ? ESC_ENABLE : ESC_DISABLE,
				keyboard_names[i]);
		putchar('\n');
		_status = status;
	}
#endif

	if (data.glitch) {
		printf(ESC_ERROR "%lu\tkeyboard: Glitch %08lx\n", systick_cnt(), data.glitch);
		data.glitch = 0;
	}

#if DEBUG >= 6
	if ((status ^ (keyboard_masks[2] | keyboard_masks[3])) == 0)
		dbgbkpt();
#endif
}

IDLE_HANDLER(&debug_keyboard);
#endif
