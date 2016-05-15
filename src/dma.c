#include <stdint.h>
#include "stm32f1xx.h"
#include "dma.h"
#include "debug.h"

void initDMA()
{
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
	uint32_t prioritygroup = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(DMA1_Channel1_IRQn, NVIC_EncodePriority(prioritygroup, 1, 0));
	NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}
