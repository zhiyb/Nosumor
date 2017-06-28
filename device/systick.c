#include <stm32f7xx.h>
#include "systick.h"
#include "clock.h"
#include "debug.h"

static volatile uint32_t cnt;

void systick_init(uint32_t hz)
{
	cnt = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
	SysTick->LOAD = ((clkAHB() << 1) / hz + 1) >> 1;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

uint32_t systick_cnt()
{
	return cnt;
}

void systick_delay(uint32_t cycles)
{
	cycles += cnt;
	while (cnt != cycles);
}

void SysTick_Handler()
{
	cnt++;
}
