#include <stm32f7xx.h>
#include <macros.h>
#include <clocks.h>
#include "rgb.h"

static void timer_rgb_init()
{
	// Initialise RGB timer
	RCC->APB1ENR |= RCC_APB1ENR_TIM5EN_Msk;
	// Auto reload buffered, centre-align mode 1, continuous
	TIM5->CR1 = TIM_CR1_ARPE_Msk | (0b01 << TIM_CR1_CMS_Pos);
	// Update event as TRGO, DMA request with CCx event
	TIM5->CR2 = (0b010 << TIM_CR2_MMS2_Pos);
	// Slave mode disabled
	TIM5->SMCR = 0;
	// DMA & interrupts disabled
	TIM5->DIER = 0;
	// OC1 mode PWM1, OC2 mode PWM1, preload enable
	TIM5->CCMR1 = (0b110 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE_Msk |
			(0b110 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE_Msk;
	// OC3 mode PWM1, preload enable
	TIM5->CCMR2 = (0b110 << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE_Msk;
	// DMA burst 3 transfers, from CCR1
	TIM5->DCR = (3 << TIM_DCR_DBL_Pos) |
			(((&TIM5->CCR1 - &TIM5->CR1) >> 2u) << TIM_DCR_DBA_Pos);
	// Input remap options
	TIM5->OR = 0;
	// Reset counter
	TIM5->CNT = 0;
	// Prescaler 120fps * 4 * (2^10) ~ 500kHz
	TIM5->PSC = clkTimer(2) / 500000ul - 1;
	// Auto reload value 0x03ff (10bit)
	TIM5->ARR = 0x03ff;
	// Output compare values
	TIM5->CCR1 = 0x01ff;
	TIM5->CCR2 = 0x02ff;
	TIM5->CCR3 = 0x03ff;
	// OC1/OC2/OC3 enable
	TIM5->CCER = TIM_CCER_CC1E_Msk | TIM_CCER_CC2E_Msk | TIM_CCER_CC3E_Msk;
	// Enable timer
	TIM5->CR1 |= TIM_CR1_CEN_Msk;
}

static void timer_scan_init()
{
	// Initialise scan timers
}

void rgb_init()
{
	// RGB:	PA0(R), PA1(G), PA2(B)			| TIM5_CH123
	// LED:	PB14(1), PB15(2), PA11(3), PA10(4)	| TIM12_CH12, TIM1_CH43
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
	// 10: Alternative function mode
	GPIO_MODER(GPIOA, 0, 0b10);
	GPIO_MODER(GPIOA, 1, 0b10);
	GPIO_MODER(GPIOA, 2, 0b10);
	// Output open drain
	GPIO_OTYPER_OD(GPIOA, 0);
	GPIO_OTYPER_OD(GPIOA, 1);
	GPIO_OTYPER_OD(GPIOA, 2);
	// Low speed (2~8MHz)
	GPIO_OSPEEDR(GPIOA, 0, 0b00);
	GPIO_OSPEEDR(GPIOA, 1, 0b00);
	GPIO_OSPEEDR(GPIOA, 2, 0b00);
	// AF2: Timer 5
	GPIO_AFRL(GPIOA, 0, 2);
	GPIO_AFRL(GPIOA, 1, 2);
	GPIO_AFRL(GPIOA, 2, 2);

	GPIO_MODER(GPIOB, 14, 0b01);
	GPIO_MODER(GPIOB, 15, 0b01);
	GPIOB->ODR |= GPIO_ODR_ODR_14 | GPIO_ODR_ODR_15;
	GPIO_MODER(GPIOA, 10, 0b01);
	GPIO_MODER(GPIOA, 11, 0b01);
	GPIOA->ODR &= ~(GPIO_ODR_ODR_10 | GPIO_ODR_ODR_11);

	timer_scan_init();
	timer_rgb_init();
}
