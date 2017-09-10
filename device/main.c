#include <stdio.h>
#include <stdint.h>
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
#include "peripheral/audio.h"
// USB interfaces
#include "usb/usb.h"
#include "usb/audio2/usb_audio2.h"
#include "usb/hid/usb_hid.h"
#include "usb/hid/keyboard/usb_hid_keyboard.h"
#include "usb/hid/vendor/usb_hid_vendor.h"
// 3rd party libraries
#include "fatfs/ff.h"
// Processing functions
#include "vendor.h"

#define BOOTLOADER_BASE	0x00260000
#define BOOTLOADER_FUNC	((void (*)())*(uint32_t *)(BOOTLOADER_BASE + 4u))

usb_t usb;	// Shared with PVD
static usb_hid_t *hid_vendor;

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
#ifndef BOOTLOADER
	bootloader_check();
#endif
	rcc_init();
	NVIC_SetPriorityGrouping(NVIC_PRIORITY_GROUPING);
	__enable_irq();
	systick_init(1000);
	usart6_init();
#ifndef BOOTLOADER
	pvd_init();
#endif

	puts(ESC_CLEAR ESC_MAGENTA VARIANT " build @ " __DATE__ " " __TIME__);
	printf(ESC_YELLOW "Core clock: " ESC_WHITE "%lu\n", clkAHB());

	puts(ESC_CYAN "Initialising USB HS...");
	usb_init(&usb, USB_OTG_HS);
	printf(ESC_YELLOW "USB in " ESC_WHITE "%s" ESC_YELLOW " mode\n",
	       usb_mode(&usb) ? "host" : "device");
	while (usb_mode(&usb) != 0);
	usb_init_device(&usb);

	puts(ESC_CYAN "Initialising USB HID interface...");
	void *hid_data = usb_hid_init(&usb);
	void *hid_keyboard = usb_hid_keyboard_init(hid_data);
	hid_vendor = usb_hid_vendor_init(hid_data);

	puts(ESC_CYAN "Initialising keyboard...");
	keyboard_init(hid_keyboard);

#ifndef BOOTLOADER
	puts(ESC_CYAN "Initialising audio...");
	audio_init();
	usb_audio2_init(&usb);
#endif

	puts(ESC_CYAN "Initialising LEDs...");
	// RGB:	PA0(R), PA1(G), PA2(B)
	// LED:	PB14(1), PB15(2), PA11(3), PA10(4)
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
	// 01: General purpose output mode
	GPIO_MODER(GPIOA, 0, 0b01);
	GPIO_MODER(GPIOA, 1, 0b01);
	GPIO_MODER(GPIOA, 2, 0b01);
	GPIOA->ODR &= ~(GPIO_ODR_ODR_0 | GPIO_ODR_ODR_1 | GPIO_ODR_ODR_2);
	GPIO_MODER(GPIOB, 14, 0b01);
	GPIO_MODER(GPIOB, 15, 0b01);
	GPIOB->ODR &= ~(GPIO_ODR_ODR_14 | GPIO_ODR_ODR_15);
	GPIO_MODER(GPIOA, 10, 0b01);
	GPIO_MODER(GPIOA, 11, 0b01);
	GPIOA->ODR &= ~(GPIO_ODR_ODR_10 | GPIO_ODR_ODR_11);

	puts(ESC_GREEN "Initialisation done");
	usb_connect(&usb, 1);
}

static inline void fatfs_test()
{
	puts(ESC_CYAN "Testing FatFS...");

	// Mount volume
	FATFS fs;
	FRESULT res;
	if ((res = f_mount(&fs, "SD:", 1)) != FR_OK) {
		printf(ESC_RED "f_mount: %d\n", res);
		return;
	}

	// Open root directory for listing
	DIR dir;
	if ((res = f_opendir(&dir, "SD:/")) != FR_OK) {
		printf(ESC_RED "f_opendir: %d\n", res);
		return;
	}
	// Iterate through directory
	FILINFO info;
	for (;;) {
		if ((res = f_readdir(&dir, &info)) != FR_OK) {
			printf(ESC_RED "f_readdir: %d\n", res);
			return;
		}
		if (info.fname[0] == 0)
			break;
		printf(ESC_WHITE "%s" ESC_YELLOW " type " ESC_WHITE "0x%x"
		       ESC_YELLOW " size " ESC_WHITE "%"PRIu64
		       "\n", info.fname, info.fattrib, info.fsize);
	}
	// Close directory
	if ((res = f_closedir(&dir)) != FR_OK) {
		printf(ESC_RED "f_closedir: %d\n", res);
		return;
	}

	// Unmount volume
	if ((res = f_mount(NULL, "SD:", 0)) != FR_OK) {
		printf(ESC_RED "f_unmount: %d\n", res);
		return;
	}

	puts(ESC_GREEN "FatFS tests completed");
}

int main()
{
	init();
#ifndef BOOTLOADER
	fatfs_test();
#endif

#ifdef DEBUG
	uint32_t mask = keyboard_masks[2] | keyboard_masks[3] | keyboard_masks[4];
#endif
	uint32_t prev_tick = systick_cnt();
	uint32_t prev_cnt = audio_transfer_cnt(), prev_data = audio_data_cnt();
	for (;;) {
		uint32_t s = keyboard_status();
		if (s & keyboard_masks[0])
			GPIOB->ODR &= ~GPIO_ODR_ODR_15;
		else
			GPIOB->ODR |= GPIO_ODR_ODR_15;
		if (s & keyboard_masks[1])
			GPIOB->ODR &= ~GPIO_ODR_ODR_14;
		else
			GPIOB->ODR |= GPIO_ODR_ODR_14;
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

		// Process time consuming tasks
		usb_process(&usb);
		audio_process();
		usb_hid_vendor_process(hid_vendor, &vendor_process);
		fflush(stdout);
#ifdef DEBUG
		if ((s & mask) == mask) {
			usb_connect(&usb, 0);
			while (keyboard_status());
			usb_connect(&usb, 1);
		}

		// Every 1024 systick ticks
		uint32_t tick = systick_cnt();
		if ((tick - prev_tick) & ~(1023ul)) {
			prev_tick += 1024ul;
			// Calculate audio frequency
			uint32_t cnt = audio_transfer_cnt();
			uint32_t diff = cnt - prev_cnt;
			prev_cnt = cnt;
			diff *= 1000ul;
			uint32_t div = 1024ul * AUDIO_FRAME_TRANSFER;
			printf(ESC_YELLOW "Audio freq: " ESC_WHITE "%lu+%lu",
			       diff / div, diff & (div - 1u));
			// Calculate audio data frequency
			cnt = audio_data_cnt();
			diff = cnt - prev_data;
			prev_data = cnt;
			diff *= 1000ul;
			div = 1024ul;
			printf(ESC_YELLOW " / " ESC_WHITE "%lu+%lu\n",
			       diff / div, diff & (div - 1u));
			// Audio volume settings
			printf(ESC_YELLOW "SP_L %ld | SP_R %ld | DAC_L %ld | DAC_R %ld\n",
			       audio_sp_vol(0), audio_sp_vol(1), audio_ch_vol(0), audio_ch_vol(1));
		}
#endif
	}
	return 0;
}
