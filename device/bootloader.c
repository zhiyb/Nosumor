#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <stm32f722xx.h>
// Miscellaneous macros and helpers
#include "macros.h"
#include "debug.h"
#include "escape.h"
#include "fio.h"
// Core peripherals
#include "clocks.h"
#include "pvd.h"
#include "irq.h"
#include "systick.h"
// Peripherals
#include "peripheral/uart.h"
#include "peripheral/keyboard.h"
#include "peripheral/mmc.h"
#include "peripheral/led.h"
// USB interfaces
#include "usb/usb.h"
#include "usb/usb_ram.h"
#include "usb/msc/usb_msc.h"
#include "usb/hid/usb_hid.h"
#include "usb/hid/keyboard/usb_hid_keyboard.h"
#include "usb/hid/vendor/usb_hid_vendor.h"
// 3rd party libraries
#include "fatfs/ff.h"
// Processing functions
#include "vendor.h"

#define BOOTLOADER_BASE	0x00260000
#define BOOTLOADER_FUNC	((void (*)())*(uint32_t *)(BOOTLOADER_BASE + 4u))

#ifdef DEBUG
extern size_t heap_usage();
extern size_t heap_size();
#endif

usb_t usb;	// Shared with PVD
static usb_msc_t *usb_msc = 0;
static usb_hid_if_t *usb_hid_vendor = 0;

static inline void bootloader_check()
{
	// Initialise GPIOs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	// PC13, PC14, PC15
	GPIO_MODER(GPIOC, 13, 0b00);
	GPIO_MODER(GPIOC, 14, 0b00);
	GPIO_MODER(GPIOC, 15, 0b00);
	GPIO_PUPDR(GPIOC, 13, GPIO_PUPDR_UP);
	GPIO_PUPDR(GPIOC, 14, GPIO_PUPDR_UP);
	GPIO_PUPDR(GPIOC, 15, GPIO_PUPDR_UP);
	if (!(GPIOC->IDR & (0b111ul << 13u))) {
		// Set reset vector
		SCB->VTOR = BOOTLOADER_BASE;
		BOOTLOADER_FUNC();
	}
}

static inline void usart6_init()
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	// 10: Alternative function mode
	GPIO_MODER(GPIOC, 6, 0b10);
	GPIO_MODER(GPIOC, 7, 0b10);
	GPIO_OTYPER_PP(GPIOC, 6);
	GPIO_OSPEEDR(GPIOC, 6, 0b00);	// Low speed (4MHz)
	GPIO_PUPDR(GPIOC, 7, GPIO_PUPDR_UP);
	// AF8: USART6
	GPIO_AFRL(GPIOC, 6, 8);
	GPIO_AFRL(GPIOC, 7, 8);
	uart_init(USART6);
	uart_config(USART6, 115200);
	fio_setup(uart_putc, uart_getc, USART6);
}

static inline void init()
{
	SCB_EnableICache();
	SCB_EnableDCache();
	rcc_init();
	NVIC_SetPriorityGrouping(NVIC_PRIORITY_GROUPING);
	__enable_irq();
	systick_init(1000);
	usart6_init();

	puts(ESC_BOOT VARIANT " build @ " __DATE__ " " __TIME__);
	printf(ESC_INFO "Core clock: " ESC_DATA "%lu\n", clkAHB());

	puts(ESC_INIT "Initialising LEDs...");
	led_init();

	puts(ESC_INIT "Initialising USB HS...");
	usb_init(&usb, USB_OTG_HS);
	printf(ESC_INFO "USB in " ESC_DATA "%s" ESC_INFO " mode\n",
	       usb_mode(&usb) ? "host" : "device");
	while (usb_mode(&usb) != 0);
	usb_init_device(&usb);

	puts(ESC_INIT "Initialising USB HID interface...");
	usb_hid_t *hid = usb_hid_init(&usb);
	usb_hid_if_t *hid_keyboard = usb_hid_keyboard_init(hid);
	usb_hid_vendor = usb_hid_vendor_init(hid);

	puts(ESC_INIT "Initialising keyboard...");
	keyboard_init(hid_keyboard);

	puts(ESC_INIT "Initialising SD/MMC card...");
	mmc_disk_init();
	puts(ESC_INIT "Initialising USB mass storage...");
	usb_msc = usb_msc_init(&usb);

	puts(ESC_GOOD "Initialisation done");
	usb_connect(&usb, 1);
}

int main()
{
	init();

#ifdef DEBUG
	uint32_t mask = keyboard_masks[2] | keyboard_masks[3] | keyboard_masks[4];
	struct {
		uint32_t tick, blocks, heap, usbram;
	} cur, diff, prev = {
		systick_cnt(), mmc_statistics(), 0, 0,
	};
#endif
loop:	;
	uint32_t s = keyboard_status();
#if 0
	int up = s & (keyboard_masks[0] | keyboard_masks[1]);
	if (up)
		GPIOA->ODR |= GPIO_ODR_ODR_10 | GPIO_ODR_ODR_11;
	else
		GPIOA->ODR &= ~(GPIO_ODR_ODR_10 | GPIO_ODR_ODR_11);
	if (s & keyboard_masks[0])
		GPIOB->ODR &= ~GPIO_ODR_ODR_15;
	else
		GPIOB->ODR |= GPIO_ODR_ODR_15;
	if (s & keyboard_masks[1])
		GPIOB->ODR &= ~GPIO_ODR_ODR_14;
	else
		GPIOB->ODR |= GPIO_ODR_ODR_14;
	int down = s & (keyboard_masks[2] | keyboard_masks[3] | keyboard_masks[4]);
	if (up && !down) {
		GPIOA->ODR &= ~(GPIO_ODR_ODR_0 | GPIO_ODR_ODR_2 | GPIO_ODR_ODR_1);
	} else {
		if (s & keyboard_masks[2])
			GPIOA->ODR &= ~GPIO_ODR_ODR_0;
		else
			GPIOA->ODR |= GPIO_ODR_ODR_0;
		if (s & keyboard_masks[3])
			GPIOA->ODR &= ~GPIO_ODR_ODR_2;
		else
			GPIOA->ODR |= GPIO_ODR_ODR_2;
		if (s & keyboard_masks[4])
			GPIOA->ODR &= ~GPIO_ODR_ODR_1;
		else
			GPIOA->ODR |= GPIO_ODR_ODR_1;
	}
#endif

	// Process time consuming tasks
	usb_process(&usb);
	usb_msc_process(&usb, usb_msc);
	usb_hid_vendor_process(usb_hid_vendor, &vendor_process);
	fflush(stdout);

#ifdef DEBUG
	if ((s & mask) == mask) {
		usb_connect(&usb, 0);
		while (keyboard_status());
		usb_connect(&usb, 1);
	}

	// Every 1024 systick ticks
	cur.tick = systick_cnt();
	if ((cur.tick - prev.tick) & ~(1023ul)) {
		// Update current values
		cur.blocks = mmc_statistics();
		cur.heap = heap_usage();
		cur.usbram = usb_ram_usage(&usb);

		// Calculate difference
		diff.blocks = cur.blocks - prev.blocks;
		diff.heap = cur.heap - prev.heap;
		diff.usbram = cur.usbram - prev.usbram;

		if (diff.heap)
			dbgprintf(ESC_INFO "[HEAP] "
				  ESC_DATA "%.2f%%" ESC_INFO ", "
				  ESC_DATA "%lu" ESC_INFO "/"
				  ESC_DATA "%u" ESC_INFO " bytes\n",
				  (float)cur.heap * 100.f / (float)heap_size(),
				  cur.heap, heap_size());

		if (diff.usbram)
			dbgprintf(ESC_INFO "[USBRAM] "
				  ESC_DATA "%.2f%%" ESC_INFO ", "
				  ESC_DATA "%lu" ESC_INFO "/"
				  ESC_DATA "%lu" ESC_INFO " bytes\n",
				  (float)cur.usbram * 100.f / (float)usb_ram_size(&usb),
				  cur.usbram, usb_ram_size(&usb));

		// SDMMC statistics
		if (diff.blocks)
			printf(ESC_INFO "[SDMMC] " ESC_DATA "%lu + %lu"
			       ESC_INFO " blocks\n", prev.blocks, diff.blocks);

		// Update previous values
		uint32_t tick = prev.tick + 1024ul;
		memcpy(&prev, &cur, sizeof(cur));
		prev.tick = tick;
	}
#endif
#ifndef DEBUG
	__WFI();
#endif
	goto loop;
	return 0;
}
