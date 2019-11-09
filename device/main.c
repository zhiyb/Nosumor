#include <stdio.h>
#include <debug.h>
#include <system/systick.h>
#include <system/system.h>
#include <peripheral/led.h>
#include <peripheral/keyboard.h>

static inline void init()
{
	printf(ESC_BOOT "%lu\tboot: " VARIANT " build @ " __DATE__ " " __TIME__ "\n", systick_cnt());
	led_init();
	keyboard_init();
}

int main()
{
	init();
	for (;;) {
		debug_keyboard();
		system_debug_process();
		__WFI();
	}
	panic();
}
