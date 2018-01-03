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
#include "peripheral/audio.h"
#include "peripheral/mmc.h"
// USB interfaces
#include "usb/usb.h"
#include "usb/audio2/usb_audio2.h"
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
#ifndef BOOTLOADER
	bootloader_check();
#endif
	SCB_EnableICache();
	SCB_EnableDCache();
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

#ifndef BOOTLOADER
	puts(ESC_CYAN "Initialising audio...");
	usb_audio_t *audio = usb_audio2_init(&usb);
	audio_init(&usb, audio);
#endif

	puts(ESC_CYAN "Initialising USB HID interface...");
	usb_hid_t *hid = usb_hid_init(&usb);
	usb_hid_if_t *hid_keyboard = usb_hid_keyboard_init(hid);
	usb_hid_vendor = usb_hid_vendor_init(hid);

	puts(ESC_CYAN "Initialising keyboard...");
	keyboard_init(hid_keyboard);

	puts(ESC_CYAN "Initialising SD/MMC card...");
	mmc_disk_init();
	printf(ESC_YELLOW "Card capacity: " ESC_WHITE "%llu"
	       ESC_YELLOW " bytes\n", (uint64_t)mmc_capacity() * 512ull);
	puts(ESC_CYAN "Initialising USB mass storage...");
	if (mmc_capacity() != 0)
		usb_msc = usb_msc_init(&usb);

	puts(ESC_CYAN "Initialising LEDs...");
	// RGB:	PA0(R), PA1(G), PA2(B)			| TIM5_CH123
	// LED:	PB14(1), PB15(2), PA11(3), PA10(4)	| TIM12_CH12, TIM1_CH43
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
	// 01: General purpose output mode
	GPIO_MODER(GPIOA, 0, 0b01);
	GPIO_MODER(GPIOA, 1, 0b01);
	GPIO_MODER(GPIOA, 2, 0b01);
	GPIO_OTYPER_OD(GPIOA, 0);
	GPIO_OTYPER_OD(GPIOA, 1);
	GPIO_OTYPER_OD(GPIOA, 2);
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
	static FATFS fs SECTION(.dtcm);
	memset(&fs, 0, sizeof(fs));
	puts(ESC_CYAN "Testing FatFS...");

	// Mount volume
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
#ifdef DEBUG
	fatfs_test();
#endif
#endif

#ifdef DEBUG
	uint32_t mask = keyboard_masks[2] | keyboard_masks[3] | keyboard_masks[4];
	struct {
		uint32_t tick, audio, data, feedback, blocks;
	} cur, diff, prev = {
		systick_cnt(), audio_transfer_cnt(),
		audio_data_cnt(), usb_audio2_feedback_cnt(), mmc_statistics(),
	};
	size_t heap = 0;
#endif
loop:	;
	uint32_t s = keyboard_status();
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

	// Process time consuming tasks
	usb_process(&usb);
	usb_msc_process(&usb, usb_msc);
	usb_hid_vendor_process(usb_hid_vendor, &vendor_process);
	audio_process();
	fflush(stdout);

#ifdef DEBUG
	if ((s & mask) == mask) {
		usb_connect(&usb, 0);
		while (keyboard_status());
		usb_connect(&usb, 1);
	}

	if (heap != heap_usage()) {
		dbgprintf(ESC_YELLOW "[HEAP]: "
			  ESC_WHITE "%.2f%%" ESC_YELLOW ", "
			  ESC_WHITE "%u/%u" ESC_YELLOW " bytes\n",
			  (float)heap_usage() * 100.f / (float)heap_size(),
			  heap_usage(), heap_size());
		heap = heap_usage();
	}

	// Every 1024 systick ticks
	cur.tick = systick_cnt();
	if ((cur.tick - prev.tick) & ~(1023ul)) {
		// Update current values
		cur.audio = audio_transfer_cnt();
		cur.data = audio_data_cnt();
		cur.feedback = usb_audio2_feedback_cnt();
		cur.blocks = mmc_statistics();

		// Calculate difference
		diff.audio = (cur.audio - prev.audio) * 1000ul;
		diff.data = (cur.data - prev.data) * 1000ul;
		diff.feedback = (cur.feedback - prev.feedback) * 1000ul;
		diff.blocks = cur.blocks - prev.blocks;

		if (diff.audio && diff.data && diff.feedback) {
			// Audio update frequency
			uint32_t div = 1024ul * AUDIO_FRAME_TRANSFER;
			printf(ESC_YELLOW "Audio freq: " ESC_WHITE "%lu+%lu",
			       diff.audio / div, diff.audio & (div - 1u));
			// Audio data frequency
			div = 1024ul;
			printf(ESC_YELLOW " / " ESC_WHITE "%lu+%lu",
			       diff.data / div, diff.data & (div - 1u));
			// Audio feedback frequency
			div = 1024ul;
			printf(ESC_YELLOW " / " ESC_WHITE "%lu+%lu",
			       diff.feedback / div, diff.feedback & (div - 1u));
			// Audio data offset
			printf(ESC_YELLOW " => " ESC_WHITE "%ld\n",
			       audio_buffering());
		}

		// SDMMC statistics
		if (diff.blocks)
			printf(ESC_YELLOW "SDMMC: " ESC_WHITE "%lu"
			       ESC_YELLOW " blocks\n", diff.blocks);

		// Update previous values
		prev.tick += 1024ul;
		prev.audio = cur.audio;
		prev.data = cur.data;
		prev.feedback = cur.feedback;
		prev.blocks = cur.blocks;
	}
#endif
	goto loop;
	return 0;
}
