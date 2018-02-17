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
#include "peripheral/mpu.h"
#include "peripheral/mmc.h"
#include "peripheral/led.h"
#include "peripheral/i2c.h"
// USB interfaces
#include "usb/usb.h"
#include "usb/usb_ram.h"
#include "usb/audio2/usb_audio2.h"
#include "usb/msc/usb_msc.h"
#include "usb/hid/usb_hid.h"
#include "usb/hid/keyboard/usb_hid_keyboard.h"
#include "usb/hid/joystick/usb_hid_joystick.h"
#include "usb/hid/vendor/usb_hid_vendor.h"
// 3rd party libraries
#include "fatfs/ff.h"
// Processing functions
#include "logic/api_proc.h"
#include "logic/led_trigger.h"

#ifdef DEBUG
extern size_t heap_usage();
extern size_t heap_size();
#endif

void *i2c;	// Shared with vendor.c
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

static inline void *i2c1_init()
{
	// Initialise I2C GPIOs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN_Msk;
	GPIO_MODER(GPIOB, 8, 0b10);	// 10: Alternative function mode
	GPIO_OTYPER_OD(GPIOB, 8);
	GPIO_OSPEEDR(GPIOB, 8, 0b00);	// Low speed (4MHz)
	GPIO_PUPDR(GPIOB, 8, GPIO_PUPDR_UP);
	GPIO_AFRH(GPIOB, 8, 4);		// AF4: I2C1

	GPIO_MODER(GPIOB, 9, 0b10);
	GPIO_OTYPER_OD(GPIOB, 9);
	GPIO_OSPEEDR(GPIOB, 9, 0b00);
	GPIO_PUPDR(GPIOB, 9, GPIO_PUPDR_UP);
	GPIO_AFRH(GPIOB, 9, 4);

	static const struct i2c_config_t config = {
		.base = I2C1,
		.rx = DMA1_Stream0, .tx = DMA1_Stream7, .rxch = 1, .txch = 1,
		.rxr = &DMA1->LIFCR, .txr = &DMA1->HIFCR,
		.rxm = DMA_LIFCR_CTCIF0_Msk | DMA_LIFCR_CHTIF0_Msk |
		DMA_LIFCR_CTEIF0_Msk | DMA_LIFCR_CDMEIF0_Msk |
		DMA_LIFCR_CFEIF0_Msk,
		.txm = DMA_HIFCR_CTCIF7_Msk | DMA_HIFCR_CHTIF7_Msk |
		DMA_HIFCR_CTEIF7_Msk | DMA_HIFCR_CDMEIF7_Msk |
		DMA_HIFCR_CFEIF7_Msk,
	};

	// Enable I2C module
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN_Msk;
	// Enable I2C DMAs
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN_Msk;
	return i2c_init(&config);
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

	puts(ESC_INIT "Initialising I2C...");
	i2c = i2c1_init();

	puts(ESC_INIT "Initialising audio...");
	if (audio_init(i2c) != 0) {
		puts(ESC_ERROR "Error initialising audio");
	} else {
		usb_audio_t *audio = usb_audio2_init(&usb);
		audio_usb_audio(&usb, audio);
	}

	puts(ESC_INIT "Initialising USB HID interface...");
	usb_hid_t *hid = usb_hid_init(&usb);
	usb_hid_vendor = usb_hid_vendor_init(hid);
	api_init(usb_hid_vendor);
	usb_hid_if_t *hid_keyboard = usb_hid_keyboard_init(hid);
	usb_hid_if_t *hid_joystick = usb_hid_joystick_init(hid);

	puts(ESC_INIT "Initialising keyboard...");
	keyboard_init(hid_keyboard, hid_joystick);

	puts(ESC_INIT "Initialising MPU...");
	if (mpu_init(i2c) != 0)
		puts(ESC_ERROR "Error initialising MPU");
	else
		mpu_usb_hid(hid_joystick);

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

int main()
{
	init();

#ifdef DEBUG
	struct {
		uint32_t tick, audio, data, feedback, blocks, mpu, heap, usbram;
	} cur, diff, prev = {
		systick_cnt(), audio_transfer_cnt(),
		audio_data_cnt(), usb_audio2_feedback_cnt(),
		mmc_statistics(), mpu_cnt(), 0, 0,
	};
#endif

	uint32_t tick = 0, tick256 = 0;
loop:	// Process time consuming tasks
	usb_process(&usb);
	usb_msc_process(&usb, usb_msc);
	audio_process();
	// Update tasks every 1 ms
	if (tick != systick_cnt()) {
		tick = systick_cnt();
		led_trigger_process();
		usb_hid_vendor_process(usb_hid_vendor, &api_process);
	}
	// Update tasks every 256 ms
	if (systick_cnt() - tick256 >= 256u) {
		tick256 = systick_cnt();
#ifdef DEBUG
#if HWVER >= 0x0100
		volatile int16_t *accel = mpu_accel_avg();
		volatile int16_t *gyro = mpu_gyro_avg();
		dbgprintf(ESC_DEBUG "[MPU] "
			  ESC_DATA "(%6d, %6d, %6d)\t(%6d, %6d, %6d)\n",
			  accel[0], accel[1], accel[2],
			  gyro[0], gyro[1], gyro[2]);
#endif
#endif
		fflush(stdout);
	}

#ifdef DEBUG
	// Every 1024 systick ticks
	cur.tick = systick_cnt();
	if ((cur.tick - prev.tick) & ~(1023ul)) {
		// Update current values
		cur.audio = audio_transfer_cnt();
		cur.data = audio_data_cnt();
		cur.feedback = usb_audio2_feedback_cnt();
		cur.blocks = mmc_statistics();
		cur.mpu = mpu_cnt();
		cur.heap = heap_usage();
		cur.usbram = usb_ram_usage(&usb);

		// Calculate difference
		diff.audio = (cur.audio - prev.audio) * 1000ul;
		diff.data = (cur.data - prev.data) * 1000ul;
		diff.feedback = (cur.feedback - prev.feedback) * 1000ul;
		diff.blocks = cur.blocks - prev.blocks;
		diff.mpu = cur.mpu - prev.mpu;
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
			printf(ESC_INFO "[MMC] " ESC_DATA "%lu + %lu"
			       ESC_INFO " blocks\n", prev.blocks, diff.blocks);

		// MPU update frequency
		if (diff.mpu)
			printf(ESC_INFO "[MPU] " ESC_DATA "%lu"
			       ESC_INFO " samples\n", diff.mpu);

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
