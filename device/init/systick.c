#include <device.h>
#include <list.h>
#include "systick.h"
#include "clocks.h"
#include "irq.h"
#include "debug.h"

LIST(systick, systick_callback_t);

static volatile uint32_t cnt;

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
	LIST_ITERATE(systick, const systick_callback_t *p, p)
		(*p)(c);
}
