#include <stm32f7xx.h>
#include <macros.h>
#include <clocks.h>
#include <debug.h>
#include <vendor_defs.h>
#include "led.h"

static uint16_t colours[LED_NUM][3] ALIGN(4) SECTION(.dtcm);

static void base_scan_init()
{
	// Initialise scan GPIOs
	// LED:	PB14(1), PB15(2), PA11(3), PA10(4)	| TIM12_CH12, TIM1_CH43
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
	// 10: Alternative function mode
	GPIO_MODER(GPIOB, 14, 0b10);
	GPIO_MODER(GPIOB, 15, 0b10);
	GPIO_MODER(GPIOA, 10, 0b10);
	GPIO_MODER(GPIOA, 11, 0b10);
	// Output push-pull
	GPIO_OTYPER_PP(GPIOB, 14);
	GPIO_OTYPER_PP(GPIOB, 15);
	GPIO_OTYPER_PP(GPIOA, 10);
	GPIO_OTYPER_PP(GPIOA, 11);
	// Low speed (2~8MHz)
	GPIO_OSPEEDR(GPIOB, 14, 0b00);
	GPIO_OSPEEDR(GPIOB, 15, 0b00);
	GPIO_OSPEEDR(GPIOA, 10, 0b00);
	GPIO_OSPEEDR(GPIOA, 11, 0b00);
	// AF9: Timer 12
	GPIO_AFRH(GPIOB, 14, 9);
	GPIO_AFRH(GPIOB, 15, 9);
	// AF1: Timer 1
	GPIO_AFRH(GPIOA, 10, 1);
	GPIO_AFRH(GPIOA, 11, 1);

	// Initialise scan timer
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN_Msk;
	// Auto reload buffered, continuous
	TIM1->CR1 = TIM_CR1_ARPE_Msk;
	TIM1->CR2 = 0;
	// Slave mode, external clock from timer 5
	TIM1->SMCR = (0 << TIM_SMCR_TS_Pos) | (0b111 << TIM_SMCR_SMS_Pos);
	// DMA & interrupts disabled
	TIM1->DIER = 0;
	// OC3 mode PWM1, OC4 mode PWM2, preload enable
	TIM1->CCMR1 = 0;
	TIM1->CCMR2 = (0b110 << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE_Msk |
			(0b111 << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE_Msk;
	// Break function disable, output enable
	TIM1->BDTR = TIM_BDTR_AOE_Msk | TIM_BDTR_MOE_Msk;
	// No prescaler
	TIM1->PSC = 0;
	// Auto reload value
	TIM1->ARR = 7;
	// Output compare values
	TIM1->CCR3 = 2;
	TIM1->CCR4 = 6;
	// Initialise registers
	TIM1->EGR = TIM_EGR_UG_Msk;
	// Reset counter
	TIM1->CNT = 5;
	// OC3/OC4 enable, active low
	TIM1->CCER = TIM_CCER_CC3E_Msk | TIM_CCER_CC4E_Msk |
			TIM_CCER_CC3P_Msk | TIM_CCER_CC4P_Msk;
	// Enable timer
	TIM1->CR1 |= TIM_CR1_CEN_Msk;

	// Initialise scan timer
	RCC->APB1ENR |= RCC_APB1ENR_TIM12EN_Msk;
	// Auto reload buffered, continuous
	TIM12->CR1 = TIM_CR1_ARPE_Msk;
	// Slave mode, external clock from timer 5
	TIM12->SMCR = (1 << TIM_SMCR_TS_Pos) | (0b111 << TIM_SMCR_SMS_Pos);
	// Interrupts disabled
	TIM12->DIER = 0;
	// OC1 mode PWM1, OC2 mode PWM2, preload enable
	TIM12->CCMR1 = (0b110 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE_Msk |
			(0b111 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE_Msk;
	// No prescaler
	TIM12->PSC = 0;
	// Auto reload value
	TIM12->ARR = 7;
	// Output compare values
	TIM12->CCR1 = 2;
	TIM12->CCR2 = 6;
	// Initialise registers
	TIM12->EGR = TIM_EGR_UG_Msk;
	// Reset counter
	TIM12->CNT = 1;
	// OC1/OC2 enable, active low
	TIM12->CCER = TIM_CCER_CC1E_Msk | TIM_CCER_CC2E_Msk |
			TIM_CCER_CC1P_Msk | TIM_CCER_CC2P_Msk;
	// Enable timer
	TIM12->CR1 |= TIM_CR1_CEN_Msk;
}

static void base_rgb_init()
{
	// Initialise RGB GPIOs
	// RGB:	PA0(R), PA1(G), PA2(B)			| TIM5_CH123
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
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

	// Initialise RGB DMA (DMA1, Stream 1, Channel 6)
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN_Msk;
	// Clear DMA flags
	DMA1->LIFCR = DMA_LIFCR_CTCIF1_Msk | DMA_LIFCR_CHTIF1_Msk |
			DMA_LIFCR_CTEIF1_Msk | DMA_LIFCR_CDMEIF1_Msk |
			DMA_LIFCR_CFEIF1_Msk;
	// Memory to peripheral, circular, 16bit -> 16bit, low priority
	DMA1_Stream1->CR = (6ul << DMA_SxCR_CHSEL_Pos) | (0b00ul << DMA_SxCR_PL_Pos) |
			(0b01ul << DMA_SxCR_MSIZE_Pos) | (0b01ul << DMA_SxCR_PSIZE_Pos) |
			(0b01ul << DMA_SxCR_DIR_Pos) | DMA_SxCR_MINC_Msk | DMA_SxCR_CIRC_Msk;
	// Peripheral address
	DMA1_Stream1->PAR = (uint32_t)&TIM5->DMAR;
	// FIFO control
	DMA1_Stream1->FCR = DMA_SxFCR_DMDIS_Msk | (0b11ul << DMA_SxFCR_FTH_Pos);
	// Memory address
	DMA1_Stream1->M0AR = (uint32_t)&colours[0][0];
	// Number of data items
	DMA1_Stream1->NDTR = sizeof(colours) / 2u;

	// Initialise RGB timer
	RCC->APB1ENR |= RCC_APB1ENR_TIM5EN_Msk;
	// Auto reload buffered, centre-align mode 1, continuous
	TIM5->CR1 = TIM_CR1_ARPE_Msk | TIM_CR1_URS_Msk | (1 << TIM_CR1_CMS_Pos);
	// Update event as TRGO, DMA request with CCx event
	TIM5->CR2 = (0b010 << TIM_CR2_MMS_Pos);
	// Slave mode disabled
	TIM5->SMCR = 0;
	// Enable DMA request from OC4, disable interrupts
	TIM5->DIER = TIM_DIER_CC4DE_Msk;
	// OC1 mode PWM1, OC2 mode PWM1, preload enable
	TIM5->CCMR1 = (0b110 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE_Msk |
			(0b110 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE_Msk;
	// OC3 mode PWM1, OC4 mode PWM1, preload enable
	TIM5->CCMR2 = (0b110 << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE_Msk |
			(0b110 << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE_Msk;
	// DMA burst 3 transfers, from CCR1
	TIM5->DCR = (2 << TIM_DCR_DBL_Pos) |
			((&TIM5->CCR1 - &TIM5->CR1) << TIM_DCR_DBA_Pos);
	// Input remap options
	TIM5->OR = 0;
	// Prescaler 120fps * 4 * 2 * (2^10) ~ 1MHz
	TIM5->PSC = clkTimer(5) / 1000000ul - 1;
	// Auto reload value 0x03ff (10bit)
	TIM5->ARR = 0x03ff;
	// Output compare values
	TIM5->CCR1 = 0x01ff;
	TIM5->CCR2 = 0x02ff;
	TIM5->CCR3 = 0x03ff;
	// Update values at next update event, when counter reaches 0
	TIM5->CCR4 = 0x03ff;
	// Reset counter
	TIM5->CNT = 0;
	// Initialise registers
	TIM5->EGR = TIM_EGR_UG_Msk;
	// Clear interrupt mask
	TIM5->SR = TIM_SR_UIF_Msk;
	// OC1/OC2/OC3 enable, active low
	TIM5->CCER = TIM_CCER_CC1E_Msk | TIM_CCER_CC2E_Msk | TIM_CCER_CC3E_Msk |
			TIM_CCER_CC1P_Msk | TIM_CCER_CC2P_Msk | TIM_CCER_CC3P_Msk;
	// Enable timer
	TIM5->CR1 |= TIM_CR1_CEN_Msk;

	// Enable DMA stream
	DMA1_Stream1->CR |= DMA_SxCR_EN_Msk;
}

void led_init()
{
	static const uint16_t clr[LED_NUM][3] = {
		// Bottom-left, RGB
		{0x3ff, 0, 0},
		// Top-right, BRG
		{0x3ff, 0, 0},
		// Top-left, BRG
		{0, 0, 0x3ff},
		// Bottom-right, RGB
		{0x3ff, 0x3ff, 0},
	};
	memcpy(&colours, &clr, sizeof(clr));

	base_scan_init();
	base_rgb_init();
}

const void *led_info(uint8_t *num)
{
	static const uint8_t info[LED_NUM][2] = {
		{Bottom | Left, (3 << 4) | 12},
		{Top | Right, (3 << 4) | 12},
		{Top | Left, (3 << 4) | 12},
		{Bottom | Right, (3 << 4) | 12},
	};
	if (num)
		*num = LED_NUM;
	return info;
}

void led_set(uint32_t i, uint32_t size, const uint16_t *c)
{
	if (i >= LED_NUM || size != 3u)
		return;
	uint16_t *p = &colours[i][0];
	// Fix for out-of-order hardware connections
	switch (i) {
	case 1:
	case 2: {
		uint16_t b = *c++;
		*p++ = *c++;
		*p++ = *c++;
		*p++ = b;
		break;
	}
	default:
		*p++ = *c++;
		*p++ = *c++;
		*p++ = *c++;
		break;
	}
}

void led_get(uint32_t i, uint32_t size, uint16_t *c)
{
	if (i >= LED_NUM || size != 3u)
		return;
	uint16_t *p = &colours[i][0];
	// Fix for out-of-order hardware connections
	switch (i) {
	case 1:
	case 2: {
		uint16_t b = *p++;
		*c++ = *p++;
		*c++ = *p++;
		*c++ = b;
		break;
	}
	default:
		*c++ = *p++;
		*c++ = *p++;
		*c++ = *p++;
		break;
	}
}
