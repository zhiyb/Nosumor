#include <device.h>
#include <debug.h>
#include <system/clocks.h>
#include <system/irq.h>
#include "systick.h"

#define MAX_HANDLERS	8

static systick_handler_t handlers[MAX_HANDLERS];
static uint32_t nhandlers;

static volatile uint32_t cnt;

void systick_init(uint32_t hz)
{
	cnt = 0;
	nhandlers = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
	SysTick->LOAD = ((clkAHB() << 1) / hz + 1) >> 1;
	// Configure interrupt priority
	uint32_t pg = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(SysTick_IRQn,
			 NVIC_EncodePriority(pg, NVIC_PRIORITY_SYSTICK, 0));
	// Enable SysTick
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

uint32_t systick_cnt()
{
	return cnt;
}

void systick_delay(uint32_t cycles)
{
	uint32_t c = cnt;
	cycles += 1;
	while (cnt - c < cycles)
		__WFI();
}

void SysTick_Handler()
{
	uint32_t c = ++cnt;
	systick_handler_t *p = handlers;
	for (uint32_t i = nhandlers; i != 0; i--)
		(*p++)(c);
}

void systick_register_handler(systick_handler_t func)
{
	if (nhandlers == MAX_HANDLERS) {
		panic();
		return;
	}

	handlers[nhandlers] = func;
	nhandlers++;
}
