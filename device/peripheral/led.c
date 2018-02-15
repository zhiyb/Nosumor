#include <string.h>
#include <stm32f7xx.h>
#include <macros.h>
#include <debug.h>
#include <vendor_defs.h>
#include <system/clocks.h>
#include "led.h"

typedef uint32_t colour_t;

static colour_t colours[LED_NUM][3] ALIGN(4) SECTION(.dtcm);

static void base_scan_init()
{
#if HWVER >= 0x0100
	// Initialise scan GPIOs
	// LED: 1(PA6), 2(PA7), 3(PA15), 4(PB3)		| TIM3_CH12, TIM2_CH12
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
	// 10: Alternative function mode
	GPIO_MODER(GPIOA, 6, 0b10);
	GPIO_MODER(GPIOA, 7, 0b10);
	GPIO_MODER(GPIOA, 15, 0b10);
	GPIO_MODER(GPIOB, 3, 0b10);
	// Output push-pull
	GPIO_OTYPER_PP(GPIOA, 6);
	GPIO_OTYPER_PP(GPIOA, 7);
	GPIO_OTYPER_PP(GPIOA, 15);
	GPIO_OTYPER_PP(GPIOB, 3);
	// Low speed (2~8MHz)
	GPIO_OSPEEDR(GPIOA, 6, 0b00);
	GPIO_OSPEEDR(GPIOA, 7, 0b00);
	GPIO_OSPEEDR(GPIOA, 15, 0b00);
	GPIO_OSPEEDR(GPIOB, 3, 0b00);
	// AF2: Timer 3
	GPIO_AFRL(GPIOA, 6, 2);
	GPIO_AFRL(GPIOA, 7, 2);
	// AF1: Timer 2
	GPIO_AFRH(GPIOA, 15, 1);
	GPIO_AFRL(GPIOB, 3, 1);

	// Initialise scan timer TIM3_CH12
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN_Msk;
	// Auto reload buffered, continuous
	TIM3->CR1 = TIM_CR1_ARPE_Msk;
	TIM3->CR2 = 0;
	// Slave mode, external clock from timer 1
	TIM3->SMCR = (0 << TIM_SMCR_TS_Pos) | (0b111 << TIM_SMCR_SMS_Pos);
	// DMA & interrupts disabled
	TIM3->DIER = 0;
	// OC1 mode PWM2, OC2 mode PWM1, preload enable
	TIM3->CCMR1 = (0b111 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE_Msk |
			(0b110 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE_Msk;
	TIM3->CCMR2 = 0;
	// No prescaler
	TIM3->PSC = 0;
	// Auto reload value
	TIM3->ARR = 7;
	// Output compare values
	TIM3->CCR1 = 6;
	TIM3->CCR2 = 2;
	// Initialise registers
	TIM3->EGR = TIM_EGR_UG_Msk;
	while (TIM3->EGR & TIM_EGR_UG_Msk);
	// Reset counter
	TIM3->CNT = 0;
	// OC1/OC2 enable, active low
	TIM3->CCER = TIM_CCER_CC1E_Msk | TIM_CCER_CC2E_Msk |
			TIM_CCER_CC1P_Msk | TIM_CCER_CC2P_Msk;
	// Enable timer
	TIM3->CR1 |= TIM_CR1_CEN_Msk;

	// Initialise scan timer TIM2_CH12
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN_Msk;
	// Auto reload buffered, continuous
	TIM2->CR1 = TIM_CR1_ARPE_Msk;
	// Slave mode, external clock from timer 1
	TIM2->SMCR = (0 << TIM_SMCR_TS_Pos) | (0b111 << TIM_SMCR_SMS_Pos);
	// Interrupts disabled
	TIM2->DIER = 0;
	// OC1 mode PWM2, OC2 mode PWM1, preload enable
	TIM2->CCMR1 = (0b111 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE_Msk |
			(0b110 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE_Msk;
	TIM2->CCMR2 = 0;
	// No prescaler
	TIM2->PSC = 0;
	// Auto reload value
	TIM2->ARR = 7;
	// Output compare values
	TIM2->CCR1 = 6;
	TIM2->CCR2 = 2;
	// Initialise registers
	TIM2->EGR = TIM_EGR_UG_Msk;
	while (TIM2->EGR & TIM_EGR_UG_Msk);
	// Reset counter
	TIM2->CNT = 4;
	// OC1/OC2 enable, active low
	TIM2->CCER = TIM_CCER_CC1E_Msk | TIM_CCER_CC2E_Msk |
			TIM_CCER_CC1P_Msk | TIM_CCER_CC2P_Msk;
	// Enable timer
	TIM2->CR1 |= TIM_CR1_CEN_Msk;
#else
	// Initialise scan GPIOs
#if HWVER == 0x0002
	// LED1: 1(PB14), 2(PB15)			| TIM12_CH12
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
#else
	// LED: 1(PB14), 2(PB15), 3(PA11), 4(PA10)	| TIM12_CH12, TIM1_CH43
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
#endif
	// 10: Alternative function mode
	GPIO_MODER(GPIOB, 14, 0b10);
	GPIO_MODER(GPIOB, 15, 0b10);
#if HWVER != 0x0002
	GPIO_MODER(GPIOA, 10, 0b10);
	GPIO_MODER(GPIOA, 11, 0b10);
#endif
	// Output push-pull
	GPIO_OTYPER_PP(GPIOB, 14);
	GPIO_OTYPER_PP(GPIOB, 15);
#if HWVER != 0x0002
	GPIO_OTYPER_PP(GPIOA, 10);
	GPIO_OTYPER_PP(GPIOA, 11);
#endif
	// Low speed (2~8MHz)
	GPIO_OSPEEDR(GPIOB, 14, 0b00);
	GPIO_OSPEEDR(GPIOB, 15, 0b00);
#if HWVER != 0x0002
	GPIO_OSPEEDR(GPIOA, 10, 0b00);
	GPIO_OSPEEDR(GPIOA, 11, 0b00);
#endif
	// AF9: Timer 12
	GPIO_AFRH(GPIOB, 14, 9);
	GPIO_AFRH(GPIOB, 15, 9);
#if HWVER != 0x0002
	// AF1: Timer 1
	GPIO_AFRH(GPIOA, 10, 1);
	GPIO_AFRH(GPIOA, 11, 1);
#endif

	// Initialise scan timer TIM12_CH12
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
#if HWVER == 0x0002
	// Auto reload value
	TIM12->ARR = 3;
	// Output compare values
	TIM12->CCR1 = 2;
	TIM12->CCR2 = 2;
#else
	// Auto reload value
	TIM12->ARR = 7;
	// Output compare values
	TIM12->CCR1 = 2;
	TIM12->CCR2 = 6;
#endif
	// Initialise registers
	TIM12->EGR = TIM_EGR_UG_Msk;
	while (TIM12->EGR & TIM_EGR_UG_Msk);
	// Reset counter
	TIM12->CNT = 0;
	// OC1/OC2 enable, active low
	TIM12->CCER = TIM_CCER_CC1E_Msk | TIM_CCER_CC2E_Msk |
			TIM_CCER_CC1P_Msk | TIM_CCER_CC2P_Msk;
	// Enable timer
	TIM12->CR1 |= TIM_CR1_CEN_Msk;

#if HWVER != 0x0002
	// Initialise scan timer TIM1_CH43
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
	while (TIM1->EGR & TIM_EGR_UG_Msk);
	// Reset counter
	TIM1->CNT = 4;
	// OC3/OC4 enable, active low
	TIM1->CCER = TIM_CCER_CC3E_Msk | TIM_CCER_CC4E_Msk |
			TIM_CCER_CC3P_Msk | TIM_CCER_CC4P_Msk;
	// Enable timer
	TIM1->CR1 |= TIM_CR1_CEN_Msk;
#endif
#endif
}

static void base_rgb_init()
{
#if HWVER < 0x0100
	// Initialise RGB GPIOs
#if HWVER == 0x0002
	// RGB2: R(PA0), B(PA1), G(PA2)			| TIM5_CH123
#else
	// RGB: R(PA0), G(PA1), B(PA2)			| TIM5_CH123
#endif
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
	// Memory to peripheral, circular, 32bit -> 32bit, low priority
	DMA1_Stream1->CR = (6ul << DMA_SxCR_CHSEL_Pos) | (0b00ul << DMA_SxCR_PL_Pos) |
			(0b10ul << DMA_SxCR_MSIZE_Pos) | (0b10ul << DMA_SxCR_PSIZE_Pos) |
			(0b01ul << DMA_SxCR_DIR_Pos) | DMA_SxCR_MINC_Msk | DMA_SxCR_CIRC_Msk;
	// Peripheral address
	DMA1_Stream1->PAR = (uint32_t)&TIM5->DMAR;
	// FIFO control
	DMA1_Stream1->FCR = DMA_SxFCR_DMDIS_Msk | (0b11ul << DMA_SxFCR_FTH_Pos);
	// Memory address
	DMA1_Stream1->M0AR = (uint32_t)&colours[0][0];
#if HWVER == 0x0003
	// Number of data items
	DMA1_Stream1->NDTR = sizeof(colours) / sizeof(colours[0][0]);
#elif HWVER == 0x0002
	// Number of data items
	DMA1_Stream1->NDTR = sizeof(colours[0]) / sizeof(colours[0][0]);
#else
#error Unsupported hardware version
#endif

	// Initialise RGB timer
	RCC->APB1ENR |= RCC_APB1ENR_TIM5EN_Msk;
	// Auto reload buffered, centre-align mode 2, continuous
	TIM5->CR1 = TIM_CR1_ARPE_Msk | TIM_CR1_URS_Msk | (2 << TIM_CR1_CMS_Pos);
	// Update event as TRGO, DMA request with CCx event
	TIM5->CR2 = (0b010 << TIM_CR2_MMS_Pos);
	// Slave mode, external clock from timer 4
	TIM5->SMCR = (2 << TIM_SMCR_TS_Pos) | (0b111 << TIM_SMCR_SMS_Pos);
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
	// Prescaler /1
	TIM5->PSC = 0;
	// Auto reload value 0x03ff (10bit)
	TIM5->ARR = 0x03ff;
	// Output compare values
	TIM5->CCR1 = 0x01ff;
	TIM5->CCR2 = 0x02ff;
	TIM5->CCR3 = 0x03ff;
	// Update values at next update event, when counter reaches 0
	TIM5->CCR4 = 0x0000;
	// Initialise registers
	TIM5->EGR = TIM_EGR_UG_Msk;
	while (TIM5->EGR & TIM_EGR_UG_Msk);
	// Reset counter
	TIM5->CNT = 0;
	// Clear interrupt mask
	TIM5->SR = TIM_SR_UIF_Msk;
	// OC1/OC2/OC3 enable, active low
	TIM5->CCER = TIM_CCER_CC1E_Msk | TIM_CCER_CC2E_Msk | TIM_CCER_CC3E_Msk |
			TIM_CCER_CC1P_Msk | TIM_CCER_CC2P_Msk | TIM_CCER_CC3P_Msk;
	// Enable timer
	TIM5->CR1 |= TIM_CR1_CEN_Msk;
#endif

#if HWVER >= 0x0100 || HWVER == 0x0002
	// Initialise RGB GPIOs
#if HWVER == 0x0002
	// RGB1: R(PA10), B(PA11)			| TIM1_CH34
#else
	// RGB: R(PA9), G(PA10), B(PA11)		| TIM1_CH234
#endif
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	// 10: Alternative function mode
#if HWVER != 0x0002
	GPIO_MODER(GPIOA, 9, 0b10);
#endif
	GPIO_MODER(GPIOA, 10, 0b10);
	GPIO_MODER(GPIOA, 11, 0b10);
	// Output open drain
#if HWVER != 0x0002
	GPIO_OTYPER_OD(GPIOA, 9);
#endif
	GPIO_OTYPER_OD(GPIOA, 10);
	GPIO_OTYPER_OD(GPIOA, 11);
	// Low speed (2~8MHz)
#if HWVER != 0x0002
	GPIO_OSPEEDR(GPIOA, 9, 0b00);
#endif
	GPIO_OSPEEDR(GPIOA, 10, 0b00);
	GPIO_OSPEEDR(GPIOA, 11, 0b00);
	// AF1: Timer 1
#if HWVER != 0x0002
	GPIO_AFRH(GPIOA, 9, 1);
#endif
	GPIO_AFRH(GPIOA, 10, 1);
	GPIO_AFRH(GPIOA, 11, 1);

	// Initialise RGB DMA (DMA2, Stream 6, Channel 0)
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN_Msk;
	// Clear DMA flags
	DMA2->HIFCR = DMA_HIFCR_CTCIF6_Msk | DMA_HIFCR_CHTIF6_Msk |
			DMA_HIFCR_CTEIF6_Msk | DMA_HIFCR_CDMEIF6_Msk |
			DMA_HIFCR_CFEIF6_Msk;
	// Memory to peripheral, circular, 32bit -> 32bit, low priority
	DMA2_Stream6->CR = (0ul << DMA_SxCR_CHSEL_Pos) | (0b00ul << DMA_SxCR_PL_Pos) |
			(0b10ul << DMA_SxCR_MSIZE_Pos) | (0b10ul << DMA_SxCR_PSIZE_Pos) |
			(0b01ul << DMA_SxCR_DIR_Pos) | DMA_SxCR_MINC_Msk | DMA_SxCR_CIRC_Msk;
	// Peripheral address
	DMA2_Stream6->PAR = (uint32_t)&TIM1->DMAR;
	// FIFO control
	DMA2_Stream6->FCR = DMA_SxFCR_DMDIS_Msk | (0b11ul << DMA_SxFCR_FTH_Pos);
#if HWVER == 0x0002
	// Memory address
	DMA2_Stream6->M0AR = (uint32_t)&colours[1][0];
	// Number of data items
	DMA2_Stream6->NDTR = 4u;
#else
	// Memory address
	DMA2_Stream6->M0AR = (uint32_t)&colours[0][0];
	// Number of data items
	DMA2_Stream6->NDTR = sizeof(colours) / sizeof(colours[0][0]);
#endif

	// Initialise RGB timer
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN_Msk;
	// Auto reload buffered, centre-align mode 2, continuous
	TIM1->CR1 = TIM_CR1_ARPE_Msk | TIM_CR1_URS_Msk | (2 << TIM_CR1_CMS_Pos);
	// Update event as TRGO, DMA request with CCx event
	TIM1->CR2 = (0b010 << TIM_CR2_MMS_Pos);
	// Slave mode, external clock from timer 4
	TIM1->SMCR = (3 << TIM_SMCR_TS_Pos) | (0b111 << TIM_SMCR_SMS_Pos);
	// Enable DMA request from OC1, disable interrupts
	TIM1->DIER = TIM_DIER_CC1DE_Msk;
	// OC1 mode PWM1, OC2 mode PWM1, preload enable
	TIM1->CCMR1 = (0b110 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE_Msk |
			(0b110 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE_Msk;
	// OC3 mode PWM1, OC4 mode PWM1, preload enable
	TIM1->CCMR2 = (0b110 << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE_Msk |
			(0b110 << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE_Msk;
	// Break function disable, output enable
	TIM1->BDTR = TIM_BDTR_AOE_Msk | TIM_BDTR_MOE_Msk;
#if HWVER == 0x0002
	// DMA burst 2 transfers, from CCR3
	TIM1->DCR = (1 << TIM_DCR_DBL_Pos) |
			((&TIM1->CCR3 - &TIM1->CR1) << TIM_DCR_DBA_Pos);
#else
	// DMA burst 3 transfers, from CCR2
	TIM1->DCR = (2 << TIM_DCR_DBL_Pos) |
			((&TIM1->CCR2 - &TIM1->CR1) << TIM_DCR_DBA_Pos);
#endif
	// Input remap options
	TIM1->OR = 0;
	// Prescaler /1
	TIM1->PSC = 0;
	// Auto reload value 0x03ff (10bit)
	TIM1->ARR = 0x03ff;
	// Output compare values
	TIM1->CCR2 = 0x01ff;
	TIM1->CCR3 = 0x02ff;
	TIM1->CCR4 = 0x03ff;
	// Update values at next update event, when counter reaches 0
	TIM1->CCR1 = 0x0000;
	// Initialise registers
	TIM1->EGR = TIM_EGR_UG_Msk;
	while (TIM1->EGR & TIM_EGR_UG_Msk);
	// Reset counter
	TIM1->CNT = 0;
	// Clear interrupt mask
	TIM1->SR = TIM_SR_UIF_Msk;
#if HWVER == 0x0002
	// OC3/OC4 enable, active low
	TIM1->CCER = TIM_CCER_CC3E_Msk | TIM_CCER_CC4E_Msk |
			TIM_CCER_CC3P_Msk | TIM_CCER_CC4P_Msk;
#else
	// OC2/OC3/OC4 enable, active low
	TIM1->CCER = TIM_CCER_CC2E_Msk | TIM_CCER_CC3E_Msk | TIM_CCER_CC4E_Msk |
			TIM_CCER_CC2P_Msk | TIM_CCER_CC3P_Msk | TIM_CCER_CC4P_Msk;
#endif
	// Enable timer
	TIM1->CR1 |= TIM_CR1_CEN_Msk;
#endif

#if HWVER == 0x0002
	// Initialise RGB GPIOs
	// LED1: G(PA15)				| TIM2_CH1
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	// 10: Alternative function mode
	GPIO_MODER(GPIOA, 15, 0b10);
	// Output open drain
	GPIO_OTYPER_OD(GPIOA, 15);
	// Low speed (2~8MHz)
	GPIO_OSPEEDR(GPIOA, 15, 0b00);
	// AF1: Timer 2
	GPIO_AFRH(GPIOA, 15, 1);

	// Initialise RGB DMA (DMA1, Stream 6, Channel 3)
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN_Msk;
	// Clear DMA flags
	DMA1->HIFCR = DMA_HIFCR_CTCIF6_Msk | DMA_HIFCR_CHTIF6_Msk |
			DMA_HIFCR_CTEIF6_Msk | DMA_HIFCR_CDMEIF6_Msk |
			DMA_HIFCR_CFEIF6_Msk;
	// Memory to peripheral, circular, 32bit -> 32bit, low priority
	DMA1_Stream6->CR = (3ul << DMA_SxCR_CHSEL_Pos) | (0b00ul << DMA_SxCR_PL_Pos) |
			(0b10ul << DMA_SxCR_MSIZE_Pos) | (0b10ul << DMA_SxCR_PSIZE_Pos) |
			(0b01ul << DMA_SxCR_DIR_Pos) | DMA_SxCR_MINC_Msk | DMA_SxCR_CIRC_Msk;
	// Peripheral address
	DMA1_Stream6->PAR = (uint32_t)&TIM2->DMAR;
	// FIFO control
	DMA1_Stream6->FCR = DMA_SxFCR_DMDIS_Msk | (0b11ul << DMA_SxFCR_FTH_Pos);
	// Memory address
	DMA1_Stream6->M0AR = (uint32_t)&colours[2][1];
	// Number of data items
	DMA1_Stream6->NDTR = 2u;

	// Initialise RGB timer
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN_Msk;
	// Auto reload buffered, centre-align mode 2, continuous
	TIM2->CR1 = TIM_CR1_ARPE_Msk | TIM_CR1_URS_Msk | (2 << TIM_CR1_CMS_Pos);
	// Enable as TRGO, DMA request with CCx event
	TIM2->CR2 = (0b001 << TIM_CR2_MMS_Pos);
	// Slave mode, external clock from timer 4
	TIM2->SMCR = (3 << TIM_SMCR_TS_Pos) | (0b111 << TIM_SMCR_SMS_Pos);
	// Enable DMA request from OC2, disable interrupts
	TIM2->DIER = TIM_DIER_CC2DE_Msk;
	// OC1 mode PWM1, OC2 mode PWM1, preload enable
	TIM2->CCMR1 = (0b110 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE_Msk |
			(0b110 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE_Msk;
	TIM2->CCMR2 = 0;
	// DMA burst 1 transfer, from CCR1
	TIM2->DCR = (0 << TIM_DCR_DBL_Pos) |
			((&TIM2->CCR1 - &TIM2->CR1) << TIM_DCR_DBA_Pos);
	// Prescaler /1
	TIM2->PSC = 0;
	// Auto reload value 0x03ff (10bit)
	TIM2->ARR = 0x03ff;
	// Output compare values
	TIM2->CCR1 = 0x01ff;
	// Update values at next update event, when counter reaches 0
	TIM2->CCR2 = 0x0000;
	// Initialise registers
	TIM2->EGR = TIM_EGR_UG_Msk;
	while (TIM2->EGR & TIM_EGR_UG_Msk);
	// Reset counter
	TIM2->CNT = 0;
	// Clear interrupt mask
	TIM2->SR = TIM_SR_UIF_Msk;
	// OC1 enable, active low
	TIM2->CCER = TIM_CCER_CC1E_Msk | TIM_CCER_CC1P_Msk;
	// Enable timer
	TIM2->CR1 |= TIM_CR1_CEN_Msk;

	// Enable DMA streams
	DMA2_Stream6->CR |= DMA_SxCR_EN_Msk;
	DMA1_Stream6->CR |= DMA_SxCR_EN_Msk;
#endif

	// Enable DMA stream
#if HWVER >= 0x0100
	DMA2_Stream6->CR |= DMA_SxCR_EN_Msk;
#else
	DMA1_Stream1->CR |= DMA_SxCR_EN_Msk;
#endif
}

static void base_psc_init()
{
	// Initialise prescaler timer TIM4
	RCC->APB1ENR |= RCC_APB1ENR_TIM4EN_Msk;
	// Auto reload buffered, edge-aligned mode, up counter, continuous
	TIM4->CR1 = TIM_CR1_ARPE_Msk | TIM_CR1_URS_Msk | (0 << TIM_CR1_CMS_Pos);
	// Update event as TRGO
	TIM4->CR2 = (0b010 << TIM_CR2_MMS_Pos);
	// Slave mode disabled
	TIM4->SMCR = 0;
	// Disable interrupts
	TIM4->DIER = 0;
	// Disable output compare channels
	TIM4->CCMR1 = 0;
	TIM4->CCMR2 = 0;
	// Prescaler /1
	TIM4->PSC = 0;
	// Auto reload value: 120fps * 4 * 2 * (2^10) ~ 1MHz
	TIM4->ARR = clkTimer(4) / 1000000ul - 1;
	// Initialise registers
	TIM4->EGR = TIM_EGR_UG_Msk;
	while (TIM4->EGR & TIM_EGR_UG_Msk);
	// Reset counter
	TIM4->CNT = 0;
	// Clear interrupt mask
	TIM4->SR = TIM_SR_UIF_Msk;
	// Disable outputs
	TIM4->CCER = 0;
	// Enable timer
	TIM4->CR1 |= TIM_CR1_CEN_Msk;
}

void led_init()
{
	static const uint16_t clr[LED_NUM][3] = {
		// Bottom-left, RGB
		{0x3ff, 0, 0},
		// Top-left, RGB
		{0, 0x3ff, 0},
		// Top-right, RGB
		{0, 0, 0x3ff},
#if LED_NUM == 4
		// Bottom-right, RGB
		{0x3ff, 0x3ff, 0},
#endif
	};
	const uint16_t (*p)[3] = clr;
	for (uint32_t i = 0; i != LED_NUM; i++)
		led_set(i, 3, *p++);

	base_scan_init();
	base_rgb_init();
	base_psc_init();
}

const void *led_info(uint8_t *num)
{
	static const uint8_t info[LED_NUM][2] = {
#if LED_NUM == 3
		{Bottom | Left | Right,	(3 << 4) | 10},
#elif LED_NUM == 4
		{Bottom | Left,		(3 << 4) | 10},
#endif
		{Top | Left,		(3 << 4) | 10},
		{Top | Right,		(3 << 4) | 10},
#if LED_NUM == 4
		{Bottom | Right,	(3 << 4) | 10},
#endif
	};
	if (num)
		*num = LED_NUM;
	return info;
}

void led_set(uint32_t i, uint32_t size, const uint16_t *c)
{
	if (i >= LED_NUM || size != 3u) {
		dbgbkpt();
		return;
	}
	colour_t *p = &colours[i][0];
	// Fix for out-of-order hardware connections
	switch (i) {
#if HWVER == 0x0003
	case 1:
	case 2: {
		colour_t *b = p++;
		*p++ = *c++;
		*p++ = *c++;
		*b = *c++;
		break;
	}
#elif HWVER == 0x0002
	case 0:
		*p++ = *c++;
		uint16_t g = *c++;
		*p++ = *c++;
		*p++ = g;
		break;
	case 1:
		colours[1][0] = *c++;
		colours[2][1] = *c++;
		colours[1][1] = *c++;
		break;
	case 2:
		colours[1][2] = *c++;
		colours[2][2] = *c++;
		colours[2][0] = *c++;
		break;
#endif
	default:
		*p++ = *c++;
		*p++ = *c++;
		*p++ = *c++;
		break;
	}
}

void led_get(uint32_t i, uint32_t size, uint16_t *c)
{
	if (i >= LED_NUM || size != 3u) {
		dbgbkpt();
		return;
	}
	colour_t *p = &colours[i][0];
	// Fix for out-of-order hardware connections
	switch (i) {
#if HWVER == 0x0003
	case 1:
	case 2: {
		uint16_t b = *p++;
		*c++ = *p++;
		*c++ = *p++;
		*c++ = b;
		break;
	}
#elif HWVER == 0x0002
	case 0:
		*c++ = *p++;
		uint16_t b = *p++;
		*c++ = *p++;
		*c++ = b;
		break;
	case 1:
		*c++ = colours[1][0];
		*c++ = colours[2][1];
		*c++ = colours[1][1];
		break;
	case 2:
		*c++ = colours[1][2];
		*c++ = colours[2][2];
		*c++ = colours[2][0];
		break;
#endif
	default:
		*c++ = *p++;
		*c++ = *p++;
		*c++ = *p++;
		break;
	}
}
