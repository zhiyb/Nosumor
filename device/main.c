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
#include "system/flash_scsi.h"
#include "irq.h"
// Peripherals
#include "peripheral/uart.h"
#include "peripheral/keyboard.h"
#include "peripheral/audio.h"
#include "peripheral/mmc.h"
#include "peripheral/led.h"
// USB interfaces
#include "usb/usb.h"
#include "usb/usb_ram.h"
#include "usb/audio2/usb_audio2.h"
#include "usb/msc/usb_msc.h"
#include "usb/hid/usb_hid.h"
#include "usb/hid/keyboard/usb_hid_keyboard.h"
#include "usb/hid/vendor/usb_hid_vendor.h"
// 3rd party libraries
#include "fatfs/ff.h"
// Processing functions
#include "logic/vendor.h"
#include "logic/led_trigger.h"

#ifdef DEBUG
extern size_t heap_usage();
extern size_t heap_size();
#endif

usb_t usb;	// Shared with PVD
static usb_msc_t *usb_msc = 0;
static usb_hid_if_t *usb_hid_vendor = 0;

void bootloader_check() {}

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
	pvd_init();

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

	puts(ESC_INIT "Initialising audio...");
	usb_audio_t *audio = usb_audio2_init(&usb);
	audio_init(&usb, audio);

	puts(ESC_INIT "Initialising USB HID interface...");
	usb_hid_t *hid = usb_hid_init(&usb);
	usb_hid_if_t *hid_keyboard = usb_hid_keyboard_init(hid);
	usb_hid_vendor = usb_hid_vendor_init(hid);

	puts(ESC_INIT "Initialising keyboard...");
	keyboard_init(hid_keyboard);

	puts(ESC_INIT "Initialising SD/MMC card...");
	mmc_disk_init();

#ifdef DEBUG
	puts(ESC_INIT "Initialising FatFs for flash...");
	uint32_t err = flash_fatfs_init(FLASH_CONF, 0);
#endif

	puts(ESC_INIT "Initialising USB mass storage...");
	usb_msc = usb_msc_init(&usb);
	usb_msc_scsi_register(usb_msc, mmc_scsi_handlers());
#ifdef DEBUG
	if (!err)
		usb_msc_scsi_register(usb_msc, flash_scsi_handlers(FLASH_CONF));
#endif

	puts(ESC_GOOD "Initialisation done");
	usb_connect(&usb, 1);
}

static inline void fatfs_test()
{
	static FATFS fs SECTION(.dtcm);
	memset(&fs, 0, sizeof(fs));
	puts(ESC_INIT "Testing FatFS...");

	// Mount volume
	FRESULT res;
	if ((res = f_mount(&fs, "MMC:", 1)) != FR_OK) {
		printf(ESC_ERROR "f_mount: %d\n", res);
		return;
	}

	// Open root directory for listing
	DIR dir;
	if ((res = f_opendir(&dir, "MMC:/")) != FR_OK) {
		printf(ESC_ERROR "f_opendir: %d\n", res);
		return;
	}
	// Iterate through directory
	FILINFO info;
	for (;;) {
		if ((res = f_readdir(&dir, &info)) != FR_OK) {
			printf(ESC_ERROR "f_readdir: %d\n", res);
			return;
		}
		if (info.fname[0] == 0)
			break;
		printf(ESC_INFO ESC_DATA "%s" ESC_INFO " type " ESC_DATA "0x%x"
		       ESC_INFO " size " ESC_DATA "%"PRIu64
		       "\n", info.fname, info.fattrib, info.fsize);
	}
	// Close directory
	if ((res = f_closedir(&dir)) != FR_OK) {
		printf(ESC_ERROR "f_closedir: %d\n", res);
		return;
	}

	// Unmount volume
	if ((res = f_mount(NULL, "MMC:", 0)) != FR_OK) {
		printf(ESC_ERROR "f_unmount: %d\n", res);
		return;
	}

	puts(ESC_GOOD "FatFS tests completed");
}

int main()
{
	init();
#ifdef DEBUG
	fatfs_test();
#endif

#ifdef DEBUG
	struct {
		uint32_t tick, audio, data, feedback, blocks, heap, usbram;
	} cur, diff, prev = {
		systick_cnt(), audio_transfer_cnt(),
		audio_data_cnt(), usb_audio2_feedback_cnt(),
		mmc_statistics(), 0, 0,
	};
#endif

loop:
	// Process time consuming tasks
	usb_process(&usb);
	usb_msc_process(&usb, usb_msc);
	usb_hid_vendor_process(usb_hid_vendor, &vendor_process);
	audio_process();
	led_trigger_process();
	fflush(stdout);

#ifdef DEBUG
	// Every 1024 systick ticks
	cur.tick = systick_cnt();
	if ((cur.tick - prev.tick) & ~(1023ul)) {
		// Update current values
		cur.audio = audio_transfer_cnt();
		cur.data = audio_data_cnt();
		cur.feedback = usb_audio2_feedback_cnt();
		cur.blocks = mmc_statistics();
		cur.heap = heap_usage();
		cur.usbram = usb_ram_usage(&usb);

		// Calculate difference
		diff.audio = (cur.audio - prev.audio) * 1000ul;
		diff.data = (cur.data - prev.data) * 1000ul;
		diff.feedback = (cur.feedback - prev.feedback) * 1000ul;
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

		if (diff.audio || diff.data || diff.feedback) {
			// Audio update frequency
			uint32_t div = 1024ul * AUDIO_FRAME_TRANSFER;
			printf(ESC_INFO "[AUDIO] " ESC_DATA "%lu+%lu",
			       diff.audio / div, diff.audio & (div - 1u));
			// Audio data frequency
			div = 1024ul;
			printf(ESC_INFO " / " ESC_DATA "%lu+%lu",
			       diff.data / div, diff.data & (div - 1u));
			// Audio feedback frequency
			div = 1024ul;
			printf(ESC_INFO " / " ESC_DATA "%lu+%lu",
			       diff.feedback / div, diff.feedback & (div - 1u));
			// Audio data offset
			printf(ESC_INFO " => " ESC_DATA "%ld\n",
			       audio_buffering());
		}

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
#if 0
	__WFE();
#endif
	goto loop;
	return 0;
}
