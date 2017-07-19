#include <stm32f7xx.h>
#include "clocks.h"
#include "macros.h"

extern uint32_t SystemCoreClock;
void SystemCoreClockUpdate(void);

uint32_t clkAHB()
{
	return SystemCoreClock;
}

uint32_t clkAPB1()
{
	uint32_t div = (RCC->CFGR & RCC_CFGR_PPRE1_Msk) >> RCC_CFGR_PPRE1_Pos;
	if (!(div & 0b100))
		return clkAHB();
	return clkAHB() / (2 << (div & 0b11));
}

uint32_t clkAPB2()
{
	uint32_t div = (RCC->CFGR & RCC_CFGR_PPRE2_Msk) >> RCC_CFGR_PPRE2_Pos;
	if (!(div & 0b100))
		return clkAHB();
	return clkAHB() / (2 << (div & 0b11));
}

static inline void mco1_init()
{
	// MCO1: HSE / 1
	RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_MCO1 | RCC_CFGR_MCO1PRE)) |
			(0b10 << RCC_CFGR_MCO1_Pos) | (0 << RCC_CFGR_MCO1PRE_Pos);
	// Configure GPIOs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	GPIO_MODER(GPIOA, 8, 0b10);	// 10: Alternative function mode
	GPIO_OTYPER_PP(GPIOA, 8);
	GPIO_OSPEEDR(GPIOA, 8, 0b01);	// Medium speed (25MHz)
	GPIO_AFRH(GPIOA, 8, 0);		// AF0: MCO1
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
	// Set dedicated clocks
	RCC->DCKCFGR2 = 0;
	while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL);
	// Update clock configuration
	SystemCoreClockUpdate();
	// Enable clock output for other chips
	mco1_init();
}

uint32_t clkSDMMC1()
{
	if (RCC->DCKCFGR2 & RCC_DCKCFGR2_SDMMC1SEL_Msk)
		return clkAHB();
	return 48ul * 1000 * 1000;
}

uint32_t clkSDMMC2()
{
	if (RCC->DCKCFGR2 & RCC_DCKCFGR2_SDMMC1SEL_Msk)
		return clkAHB();
	return 48ul * 1000 * 1000;
}
