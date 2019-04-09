#include <device.h>
#include <irq.h>
#include "pvd.h"

void pvd_init()
{
	RCC->APB1ENR |= RCC_APB1ENR_PWREN_Msk;
	// PVD level: 2.8V
	PWR->CR1 = (PWR->CR1 & ~(PWR_CR1_PLS_Msk)) | (0b110 << PWR_CR1_PLS_Pos);
	// Enable programmable voltage detector
	PWR->CR1 |= PWR_CR1_PVDE_Msk;
	// Setup interrupts
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN_Msk;
	// Rising and falling edge trigger
	EXTI->RTSR |= EXTI_RTSR_TR16_Msk;
	EXTI->FTSR &= ~EXTI_FTSR_TR16_Msk;
	// Clear interrupts
	EXTI->PR = EXTI_PR_PR16_Msk;
	// Unmask interrupts
	EXTI->IMR |= EXTI_PR_PR16_Msk;
	// Configure interrupt priority
	uint32_t pg = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(PVD_IRQn, NVIC_EncodePriority(pg, NVIC_PRIORITY_PVD));
	NVIC_EnableIRQ(PVD_IRQn);

	// GPIO PB7 output for monitoring
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	GPIO_MODER(GPIOB, 7, GPIO_MODER_OUTPUT);
	GPIOB->ODR &= ~GPIO_ODR_ODR_7;
}

void PVD_IRQHandler()
{
	GPIOB->ODR |= GPIO_ODR_ODR_7;
}
