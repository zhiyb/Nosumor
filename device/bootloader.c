#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <stm32f722xx.h>
// Miscellaneous macros and helpers
#include "logic/fio.h"
#include "macros.h"
#include "debug.h"
#include "escape.h"
// Core peripherals
#include "system/clocks.h"
#include "system/pvd.h"
#include "system/systick.h"
#include "system/flash.h"
#include "system/flash_scsi.h"
#include "irq.h"
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
#include "api/api_proc.h"

#define RESET_ENTRY(b)	((void (*)())*(uint32_t *)((uint32_t)(b) + 4u))

#ifdef DEBUG
size_t heap_usage();
size_t heap_size();
#endif

extern uint32_t __appi_start__;

usb_t usb;	// Shared with PVD
static usb_msc_t *usb_msc = 0;
static usb_hid_if_t *usb_hid_vendor = 0;

void bootloader_check()
{
	// Check app region status
	if (__appi_start__ != 0)
		return;
	// Initialise GPIOs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	// PC13, PC14, PC15
	GPIO_MODER(GPIOC, 13, 0b00);
	GPIO_MODER(GPIOC, 14, 0b00);
	GPIO_MODER(GPIOC, 15, 0b00);
	GPIO_PUPDR(GPIOC, 13, GPIO_PUPDR_UP);
	GPIO_PUPDR(GPIOC, 14, GPIO_PUPDR_UP);
	GPIO_PUPDR(GPIOC, 15, GPIO_PUPDR_UP);
	// Jump to app if no button pressed
	if (GPIOC->IDR & (0b111ul << 13u)) {
		// Set reset vector
		SCB->VTOR = (uint32_t)&__appi_start__;
		RESET_ENTRY(&__appi_start__)();
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
	for (uint8_t i = LED_NUM; i--;)
		led_set(i, 3, (const uint16_t[3]){0x03ff, 0, 0});

	puts(ESC_INIT "Initialising USB HS...");
	usb_init(&usb, USB_OTG_HS);
	printf(ESC_INFO "USB in " ESC_DATA "%s" ESC_INFO " mode\n",
	       usb_mode(&usb) ? "host" : "device");
	while (usb_mode(&usb) != 0);
	usb_init_device(&usb);

	puts(ESC_INIT "Initialising USB HID interface...");
	usb_hid_t *hid = usb_hid_init(&usb);
	usb_hid_vendor = usb_hid_vendor_init(hid);
	api_init(usb_hid_vendor);
	usb_hid_if_t *hid_keyboard = usb_hid_keyboard_init(hid);

	puts(ESC_INIT "Initialising keyboard...");
	keyboard_init(hid_keyboard, 0, 0);

	puts(ESC_INIT "Initialising FatFs for flash...");
	if (flash_fatfs_init(FLASH_CONF, 0))
		dbgbkpt();
	if (flash_fatfs_init(FLASH_APP, 1))
		dbgbkpt();

	puts(ESC_INIT "Initialising USB mass storage...");
	usb_msc = usb_msc_init(&usb);
	usb_msc_scsi_register(usb_msc, mmc_scsi_handlers());
	usb_msc_scsi_register(usb_msc, flash_scsi_handlers(FLASH_CONF));
	usb_msc_scsi_register(usb_msc, flash_scsi_handlers(FLASH_APP));

	puts(ESC_GOOD "Initialisation done");
	usb_connect(&usb, 1);

	for (uint8_t i = LED_NUM; i--;)
		led_set(i, 3, (const uint16_t[3]){0, 0, 0x03ff});
}

static inline void flash()
{
	FRESULT res;
	FATFS fs;
	memset(&fs, 0, sizeof(fs));

	// Mount volume
	if ((res = f_mount(&fs, "APP:", 1)) != FR_OK) {
		printf(ESC_ERROR "[BL] Mount failed: " ESC_DATA "%u\n", res);
		return;
	}

	// Find firmware
	DIR dir;
	FILINFO fno;
	if ((res = f_findfirst(&dir, &fno, "APP:", "*.hex")) != FR_OK) {
		printf(ESC_ERROR "[BL] Failure finding firmware: "
		       ESC_DATA "%u\n", res);
		return;
	}

	if (!fno.fname[0]) {
		dbgprintf(ESC_ERROR "[BL] Firmware not found: "
			  ESC_DATA "%u\n", res);
		return;
	}

	printf(ESC_INFO "[BL] Firmware " ESC_DATA "%s"
	       ESC_INFO ", size " ESC_DATA "%lu\n",
	       fno.fname, (uint32_t)fno.fsize);

	char path[5 + strlen(fno.fname)];
	memcpy(path, "APP:", 4);
	strcpy(path + 4u, fno.fname);
	fflush(stdout);
	flash_fatfs_hex_program(path);
}

int main()
{
	init();

	uint32_t tick = 0, wrsize = 0;
loop:
	// Wait until more than 100kB has been written to app flash
	if (!flash_busy() && wrsize >= 100 * 1024) {
		// Check flash statistics every 3 seconds
		if ((int)(systick_cnt() - tick) >= 3 * 1000) {
			tick = systick_cnt();
			uint32_t diff = flash_stat_write(FLASH_APP) - wrsize;
			if (!diff) {
#ifdef DEBUG
				printf(ESC_INFO "[FLASH APP] Read: "
				       ESC_DATA "%lu" ESC_INFO " bytes\n",
				       flash_stat_read(FLASH_APP));
				printf(ESC_INFO "[FLASH APP] Write: "
				       ESC_DATA "%lu" ESC_INFO " bytes\n",
				       flash_stat_write(FLASH_APP));
				fflush(stdout);
#endif
				// Start flashing
				flash();
			} else {
				wrsize = flash_stat_write(FLASH_APP);
			}
		}
	} else {
		tick = systick_cnt();
		wrsize = flash_stat_write(FLASH_APP);
	}

	// Process time consuming tasks
	usb_process(&usb);
	usb_msc_process(&usb, usb_msc);
	usb_hid_vendor_process(usb_hid_vendor, &api_process);
	fflush(stdout);

	goto loop;
	return 0;
}
