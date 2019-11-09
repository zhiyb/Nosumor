#include <device.h>
#include <macros.h>

extern uint32_t SystemCoreClock;
void SystemCoreClockUpdate(void);

uint32_t clkAHB()
{
	return 216000000;		// TODO calculate?
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

uint32_t clkSDMMC1()
{
	if (RCC->DCKCFGR2 & RCC_DCKCFGR2_SDMMC1SEL_Msk)
		return clkAHB();
	return 54ul * 1000 * 1000;
}

uint32_t clkSDMMC2()
{
	if (RCC->DCKCFGR2 & RCC_DCKCFGR2_SDMMC2SEL_Msk)
		return clkAHB();
	return 54ul * 1000 * 1000;
}

uint32_t clkTimer(uint32_t i)
{
	uint32_t pre = RCC->DCKCFGR1 & RCC_DCKCFGR1_TIMPRE_Msk;
	uint32_t div, clk;
	switch (i) {
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 12:
	case 13:
	case 14:
		div = FIELD(RCC->CFGR, RCC_CFGR_PPRE1);
		clk = clkAPB1();
		break;
	case 1:
	case 8:
	case 9:
	case 10:
	case 11:
		div = FIELD(RCC->CFGR, RCC_CFGR_PPRE2);
		clk = clkAPB2();
		break;
	default:
		return 0;
	}
	if (pre)
		return (div > 0b101) ? clk << 2u : clk;
	else
		return (div & 0b100) ? clk << 1u : clk;
}

static inline void mco1_init()
{
	// MCO1: HSE / 1
	RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_MCO1 | RCC_CFGR_MCO1PRE)) |
			(0b10 << RCC_CFGR_MCO1_Pos) | (0 << RCC_CFGR_MCO1PRE_Pos);
	// Enable IO compensation cell
	if (!(SYSCFG->CMPCR & SYSCFG_CMPCR_READY)) {
		RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
		SYSCFG->CMPCR |= SYSCFG_CMPCR_CMP_PD;
	}
	// Configure GPIOs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	// GPIO AF0: MCO1
	GPIO_MODER(GPIOA, 8, GPIO_MODER_ALTERNATE);
	GPIO_OTYPER_PP(GPIOA, 8);
	GPIO_OSPEEDR(GPIOA, 8, GPIO_OSPEEDR_MEDIUM);
	GPIO_AFRH(GPIOA, 8, 0);
	// Wait for IO compensation cell
	while (!(SYSCFG->CMPCR & SYSCFG_CMPCR_READY));
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
	// Configure PLL (HSE, PLLM = 12, PLLN = 270, PLLP = 2, PLLQ = 8)
	RCC->PLLCFGR = (12u << RCC_PLLCFGR_PLLM_Pos) | (270u << RCC_PLLCFGR_PLLN_Pos) |
			(0u << RCC_PLLCFGR_PLLP_Pos) | (8u << RCC_PLLCFGR_PLLQ_Pos) |
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
	// Clock output for other chips
	mco1_init();
}
