#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stm32f722xx.h>
#include "debug.h"
#include "escape.h"
#include "clock.h"
#include "fio.h"
#include "peripheral/uart.h"

// RGB2_RGB:	PA0(R), PA1(G), PA2(B)
// RGB1_RGB:	PA11(R), PA15(G), PA10(B)
// RGB1_LR:	PB14(L), PB15(R)
// KEY_12:	PA6(K1), PB2(K2)
// KEY_345:	PC13(K3), PC14(K4), PC15(K5)

void usart6_init()
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	// 10: Alternative function mode
	GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (0b10 << GPIO_MODER_MODER6_Pos);
	GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (0b10 << GPIO_MODER_MODER7_Pos);
	// AF8: USART6
	GPIOC->AFR[0] = (GPIOC->AFR[0] & ~GPIO_AFRL_AFRL6_Msk) | (8 << GPIO_AFRL_AFRL6_Pos);
	GPIOC->AFR[0] = (GPIOC->AFR[0] & ~GPIO_AFRL_AFRL7_Msk) | (8 << GPIO_AFRL_AFRL7_Pos);
	uart_init(USART6);
	uart_config(USART6, 115200);
	fio_setup(uart_putc, uart_getc, USART6);
}

void rcc_init()
{
	// Enable HSE
	RCC->CR |= RCC_CR_HSEON;
	while (!(RCC->CR & RCC_CR_HSERDY));
	// Switch to HSE
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_HSE;
	while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_HSE);
	// Disable HSI
	RCC->CR &= ~RCC_CR_HSION;
	// Disable PLL
	RCC->CR &= ~RCC_CR_PLLON;
	while (RCC->CR & RCC_CR_PLLRDY);
	// Configure PLL (HSE, PLLM = 12, PLLN = 270, PLLP = 2, PLLQ = 9)
	RCC->PLLCFGR = (12 << RCC_PLLCFGR_PLLM_Pos) | (270 << RCC_PLLCFGR_PLLN_Pos) |
			(0 << RCC_PLLCFGR_PLLP_Pos) | (9 << RCC_PLLCFGR_PLLQ_Pos) |
			RCC_PLLCFGR_PLLSRC_HSE;
	// Enable power controller
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	// Regulator voltage scale 1
	PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | (0b11 << PWR_CR1_VOS_Pos);
	// Enable PLL
	RCC->CR |= RCC_CR_PLLON;
	// Enable Over-drive
	PWR->CR1 |= PWR_CR1_ODEN;
	while (!(PWR->CSR1 & PWR_CSR1_ODRDY));
	PWR->CR1 |= PWR_CR1_ODSWEN;
	while (!(PWR->CSR1 & PWR_CSR1_ODSWRDY));
	// Set flash latency
	// ART enable, prefetch enable, 7 wait states
	FLASH->ACR = FLASH_ACR_ARTEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_7WS;
	// Set AHB & APB prescalers
	// AHB = 1, APB1 = 4, APB2 = 2
	RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) |
			RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;
	// Wait for PLL lock
	while (!(RCC->CR & RCC_CR_PLLRDY));
	// Switch to PLL
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
	while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL);
	// Update clock configuration
	SystemCoreClockUpdate();
}

int main()
{
	rcc_init();
	usart6_init();

	puts(ESC_CLEAR ESC_CYAN VARIANT " build @ " __DATE__ " " __TIME__);
	printf(ESC_YELLOW "Core clock: " ESC_WHITE "%lu\n", clkAHB());

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
	// 01: General purpose output mode
	GPIOA->MODER |= 0b01 << GPIO_MODER_MODER0_Pos;
	GPIOA->MODER |= 0b01 << GPIO_MODER_MODER1_Pos;
	GPIOA->MODER |= 0b01 << GPIO_MODER_MODER2_Pos;
	GPIOA->ODR &= ~(GPIO_ODR_ODR_0 | GPIO_ODR_ODR_1 | GPIO_ODR_ODR_2);
	GPIOB->MODER |= 0b01 << GPIO_MODER_MODER14_Pos;
	GPIOB->MODER |= 0b01 << GPIO_MODER_MODER15_Pos;
	GPIOB->ODR |= (GPIO_ODR_ODR_14 | GPIO_ODR_ODR_15);
	GPIOB->ODR &= ~(GPIO_ODR_ODR_15);
	GPIOB->ODR &= ~(GPIO_ODR_ODR_14);
	GPIOA->MODER |= 0b01 << GPIO_MODER_MODER10_Pos;
	GPIOA->MODER |= 0b01 << GPIO_MODER_MODER11_Pos;
	GPIOA->MODER |= 0b01 << GPIO_MODER_MODER15_Pos;
	GPIOA->MODER &= ~(0b10 << GPIO_MODER_MODER15_Pos);
	GPIOA->ODR &= ~(GPIO_ODR_ODR_10 | GPIO_ODR_ODR_11 | GPIO_ODR_ODR_15);
	// 01: Pull-up
	GPIOA->PUPDR |= 0b01 << GPIO_PUPDR_PUPDR6_Pos;
	GPIOB->PUPDR |= 0b01 << GPIO_PUPDR_PUPDR2_Pos;
	GPIOC->PUPDR |= 0b01 << GPIO_PUPDR_PUPDR13_Pos;
	GPIOC->PUPDR |= 0b01 << GPIO_PUPDR_PUPDR14_Pos;
	GPIOC->PUPDR |= 0b01 << GPIO_PUPDR_PUPDR15_Pos;
	for (;;) {
		if (!(GPIOA->IDR & GPIO_IDR_IDR_6))
			GPIOA->ODR |= GPIO_ODR_ODR_15;
		if (!(GPIOB->IDR & GPIO_IDR_IDR_2))
			GPIOA->ODR |= GPIO_ODR_ODR_11;
		if (GPIOC->IDR & GPIO_IDR_IDR_13)
			GPIOA->ODR |= (GPIO_ODR_ODR_0 | GPIO_ODR_ODR_1 | GPIO_ODR_ODR_2);
		else
			GPIOA->ODR &= ~(GPIO_ODR_ODR_0 | GPIO_ODR_ODR_1 | GPIO_ODR_ODR_2);
		if (GPIOC->IDR & GPIO_IDR_IDR_14)
			GPIOB->ODR |= (GPIO_ODR_ODR_14);
		else
			GPIOB->ODR &= ~(GPIO_ODR_ODR_14);
		if (GPIOC->IDR & GPIO_IDR_IDR_15)
			GPIOB->ODR |= (GPIO_ODR_ODR_15);
		else
			GPIOB->ODR &= ~(GPIO_ODR_ODR_15);
	}
	return 0;
}
