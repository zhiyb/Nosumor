#include <stdio.h>
#include <debug.h>
#include <system/systick.h>
#include <system/system.h>
#include <peripheral/led.h>
#include <peripheral/keyboard.h>
#include <usb/usb.h>

static inline void init()
{
	printf(ESC_BOOT "%lu\tboot: " VARIANT " build @ " __DATE__ " " __TIME__ "\n", systick_cnt());
	printf(ESC_INIT "%lu\tboot: Initialising peripherals\n", systick_cnt());
	led_init();
	keyboard_init();
	printf(ESC_INIT "%lu\tboot: Initialising USB system\n", systick_cnt());
	usb_init();
	printf(ESC_INIT "%lu\tboot: Soft connect USB port\n", systick_cnt());
#if !DEBUG
	usb_connect(1);
#endif
	printf(ESC_INIT "%lu\tboot: Initialisation done\n", systick_cnt());
}

static inline void debug()
{
	static uint32_t _status = 0;
	uint32_t status = keyboard_status();

	if ((status & ~_status) & keyboard_masks[3]) {
		uint32_t connect = !usb_connected();
		usb_connect(connect);
	}

	_status = status;
}

int main()
{
	init();
	for (;;) {
#if DEBUG
		debug_keyboard();
		debug();
#endif
		system_debug_process();
		__WFI();
	}
	panic();
}
