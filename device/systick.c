#include <malloc.h>
#include <stm32f7xx.h>
#include "systick.h"
#include "clocks.h"
#include "irq.h"
#include "debug.h"

static volatile uint32_t cnt;
static systick_handler_t *handlers;
static uint32_t nhandlers;

void systick_init(uint32_t hz)
{
	cnt = 0;
	handlers = 0;
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
	if (cycles == 0)
		return;
	while (cnt - c < cycles);
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
	uint32_t n = nhandlers + 1u;
	__disable_irq();
	handlers = realloc(handlers, n * sizeof(systick_handler_t));
	__enable_irq();
	*(handlers + nhandlers) = func;
	nhandlers = n;
}
