#include <stdio.h>
#include <errno.h>
#include <module.h>
#include <device.h>
#include <debug.h>
#include <clocks.h>
#include <irq.h>
#include "pvd.h"
#include "systick.h"

static void *handler(void *inst, uint32_t msg, void *data);

MODULE("init", 0, 0, handler);

const module_t *module_init = &__module__13;

static void init()
{
	SCB_EnableICache();
	SCB_EnableDCache();
	NVIC_SetPriorityGrouping(NVIC_PRIORITY_GROUPING);

	rcc_init();
	pvd_init();
	systick_init(1000);
}

static void start()
{
	__enable_irq();
	systick_enable(1);

	// Find a UART and set to stdio
	const void *pio = MODULE_FIND("uart");
	if (pio) {
		MODULE_MSG(pio, "config", 115200);
		MODULE_MSG(pio, "stdio", 0);
	}

	puts(ESC_BOOT VARIANT " build @ " __DATE__ " " __TIME__);
	printf(ESC_INFO "Core clock: " ESC_DATA "%lu\n", clkAHB());
#ifdef DEBUG
	hash_check();
#endif
}

static void *handler(void *inst, uint32_t msg, void *data)
{
	UNUSED(inst);
	if (msg == HASH("tick.get")) {
		return (void *)systick_cnt();
	} else if (msg == HASH("tick.handler.install")) {
		systick_register_handler(data);
		return 0;
	} else if (msg == HASH("init")) {
		init();
		return 0;
	} else if (msg == HASH("start")) {
		start();
		return 0;
	} else if (msg == HASH("mco1")) {
		mco1_enable((uint32_t)data);
		return data;
	} else if (msg == HASH("tick.delay")) {
		systick_delay((uint32_t)data);
		return data;
	}
	return 0;
}
