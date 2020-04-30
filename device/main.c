#include <stdio.h>
#include <debug.h>
#include <device.h>
#include <system/systick.h>
#include <system/system.h>
#include <peripheral/keyboard.h>
#include <usb/usb.h>

LIST(init, basic_handler_t);
LIST(idle, basic_handler_t);

static inline void init()
{
	printf(ESC_BOOT "%lu\tboot: " VARIANT " build @ " __DATE__ " " __TIME__ "\n", systick_cnt());
#if DEBUG >= 6
	extern void mem_test();
	mem_test();
#endif
	printf(ESC_INIT "%lu\tboot: Initialising peripherals\n", systick_cnt());
	LIST_ITERATE(init, basic_handler_t, p) (*p)();
	printf(ESC_INIT "%lu\tboot: Initialising USB systems\n", systick_cnt());
	usb_init();
#if DEBUG < 2
	printf(ESC_INIT "%lu\tboot: Soft connect USB port\n", systick_cnt());
	usb_connect(1);
#endif
	printf(ESC_INIT "%lu\tboot: Initialisation done\n", systick_cnt());
}

#if DEBUG >= 2
static void debug()
{
	static uint32_t _status = 0;
	uint32_t status = keyboard_status();

	if ((status & ~_status) & keyboard_masks[3]) {
		uint32_t connect = !usb_connected();
		usb_connect(connect);
	}

	_status = status;
}

IDLE_HANDLER(&debug);
#endif

int main()
{
	init();
	for (;;) {
		LIST_ITERATE(idle, basic_handler_t, p) (*p)();
		__WFI();
	}
	panic();
}
