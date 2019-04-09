#include <device.h>
#include "systick.h"
#include "clocks.h"
#include "irq.h"
#include "debug.h"

#define SYSTICK_MAX_HANDLERS	4

static volatile uint32_t cnt;
static systick_handler_t handlers[SYSTICK_MAX_HANDLERS] = {0};

void systick_init(uint32_t hz)
{
	cnt = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
	SysTick->LOAD = ((clkAHB() << 1) / hz + 1) >> 1;
	// Configure interrupt priority
	uint32_t pg = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(SysTick_IRQn,
			 NVIC_EncodePriority(pg, NVIC_PRIORITY_SYSTICK, 0));
	systick_enable(0);
}

void systick_enable(uint32_t e)
{
	if (e)
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	else
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

uint32_t systick_cnt()
{
	return cnt;
}

void systick_delay(uint32_t cycles)
{
	uint32_t c = cnt;
	if (cycles == 0)
		return;
	while (cnt - c < cycles)
		__WFI();
}

void SysTick_Handler()
{
	uint32_t c = ++cnt;
	systick_handler_t *p = handlers;
	for (; p != &handlers[SYSTICK_MAX_HANDLERS] && *p; p++)
		(*p)(c);
}

void systick_register_handler(systick_handler_t func)
{
	systick_handler_t *p = handlers;
	for (; p != &handlers[SYSTICK_MAX_HANDLERS] && *p; p++);
	while (p == &handlers[SYSTICK_MAX_HANDLERS])
		dbgbkpt();
	*p++ = func;
	if (p != &handlers[SYSTICK_MAX_HANDLERS])
		*p = 0;
}
