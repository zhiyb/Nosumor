#include <stdint.h>
#include <ctype.h>
#include "debug.h"
#include "escape.h"
#include <stm32f722xx.h>

// RGB2_RGB:	PA0(R), PA1(G), PA2(B)
// RGB1_RGB:	PA11(R), PA15(G), PA10(B)
// RGB1_LR:	PB14(L), PB15(R)
// KEY_12:	PA6(K1), PB2(K2)
// KEY_345:	PC13(K3), PC14(K4), PC15(K5)

extern uint32_t SystemCoreClock;
void SystemCoreClockUpdate(void);

uint32_t clkAPB2()
{
	uint32_t div = (RCC->CR & RCC_CFGR_PPRE2_Msk) >> RCC_CFGR_PPRE2_Pos;
	if (!(div & 0b100))
		return SystemCoreClock;
	return SystemCoreClock / (2 << (div & 0b11));
}

void usart6Init(unsigned long baud)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	// 10: Alternative function mode
	GPIOC->MODER |= 0b10 << GPIO_MODER_MODER6_Pos;
	GPIOC->MODER |= 0b10 << GPIO_MODER_MODER7_Pos;
	// 01: Pull-up
	GPIOC->PUPDR |= 0b01 << GPIO_PUPDR_PUPDR6_Pos;
	GPIOC->PUPDR |= 0b01 << GPIO_PUPDR_PUPDR7_Pos;
	// AF8: USART6
	GPIOC->AFR[0] |= 8 << GPIO_AFRL_AFRL6_Pos;
	GPIOC->AFR[0] |= 8 << GPIO_AFRL_AFRL7_Pos;
	RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
	unsigned long clk = clkAPB2();
	USART6->CR1 = 0;	// Disable USART
	USART6->CR1 = 0;	// 8-bit, N
	USART6->CR2 = 0;	// 1 stop
	USART6->CR3 = 0;
	USART6->BRR = clk / baud;
	// Enable transmitter & receiver & USART
	USART6->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void usart6WriteChar(const char c)
{
	while (!(USART6->ISR & USART_ISR_TXE));
	USART6->TDR = c;
}

void usart6WriteString(const char *str)
{
	char c;
	while ((c = *str++) != 0)
		usart6WriteChar(c);
}

void usart6DumpHex(const uint32_t v)
{
	int shift = 32 / 4;
	while (shift--) {
		uint8_t b = 0x0f & (v >> (shift * 4));
		if (b >= 0x0a)
			usart6WriteChar('a' + b - 0x0a);
		else
			usart6WriteChar('0' + b);
	}
}

int main()
{
	SystemCoreClockUpdate();
	usart6Init(115200);
	usart6WriteString("\r\n" ESC_CYAN "Hello, world!\r\n" ESC_DEFAULT);
	usart6WriteString(ESC_YELLOW "Core clock: " ESC_WHITE "0x");
	usart6DumpHex(SystemCoreClock);
	usart6WriteString(ESC_DEFAULT "\r\n");
	usart6WriteString(ESC_MAGENTA "Enabling HSE...\r\n");
	RCC->CR |= RCC_CR_HSEON;
	while (!(RCC->CR & RCC_CR_HSERDY));
	usart6WriteString(ESC_MAGENTA "HSE enabled, switching system clock...\r\n");
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_HSE;
	while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_HSE);
	SystemCoreClockUpdate();
	usart6Init(115200);
	usart6WriteString(ESC_GREEN "\r\nSuccess!\r\n");
	usart6WriteString(ESC_YELLOW "Core clock: " ESC_WHITE "0x");
	usart6DumpHex(SystemCoreClock);
	usart6WriteString(ESC_DEFAULT "\r\n");
	for (;;)
		if (USART6->ISR & USART_ISR_RXNE)
			usart6WriteChar(toupper(USART6->RDR));

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
