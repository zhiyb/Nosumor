#include <system/clocks.h>
#include "uart.h"

void uart_init(USART_TypeDef *uart)
{
	switch ((uint32_t)uart) {
	case (uint32_t)USART6:
		// Enable USART6 clock
		RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
		break;
	}
}

void uart_config(USART_TypeDef *uart, uint32_t baud)
{
	uint32_t clk;
	switch ((uint32_t)uart) {
	case (uint32_t)USART1:
	case (uint32_t)USART6:
		clk = clkAPB2();
		break;
	default:
		clk = clkAPB1();
	}
	uart->CR1 = 0;	// Disable USART
	uart->CR1 = 0;	// 8-bit, N
	uart->CR2 = 0;	// 1 stop
	uart->CR3 = 0;
	uart->BRR = ((clk << 1) / baud + 1) >> 1;
	// Enable transmitter & receiver & USART
	uart->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void uart_putc(void *base, char c)
{
	USART_TypeDef *uart = base;
	while (!(uart->ISR & USART_ISR_TXE));
	uart->TDR = c;
}

char uart_getc(void *base)
{
	USART_TypeDef *uart = base;
	while (!(uart->ISR & USART_ISR_RXNE));
	return uart->RDR;
}
