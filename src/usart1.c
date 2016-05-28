#include <stdint.h>
#include "usart1.h"
#include "macros.h"
#include "debug.h"

// Port A, lower byte
#define UART_GPIO	GPIOA
#define UART_TX		9
#define UART_RX		10

#define UART_CLK	72000000UL
#define UART_BAUD	4000000UL

// baud = fck / (16 * USARTDIV)
#define USART_BRR(clk, baud)	((((clk) << 1) / (baud) + 1) >> 1)

void initUSART1()
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

	// 10, 01, x: Alternate function output push-pull, 10MHz
	HL(UART_GPIO->CRL, UART_TX) &= ~(0x06 << (MOD8(UART_TX) << 2));
	HL(UART_GPIO->CRL, UART_TX) |= 0x09 << (MOD8(UART_TX) << 2);

	// 10, 00, 1: Input with pull-up
	HL(UART_GPIO->CRL, UART_RX) &= ~(0x07 << (MOD8(UART_RX) << 2));
	HL(UART_GPIO->CRL, UART_RX) |= 0x08 << (MOD8(UART_RX) << 2);
	UART_GPIO->BSRR = BV(UART_RX);

	// USART1 no remap
	AFIO->MAPR &= ~AFIO_MAPR_USART1_REMAP;

	// USART_BAUD, 8n1
	USART1->CR1 = USART_CR1_TE | USART_CR1_RE;
	USART1->CR2 = 0;
	USART1->CR3 = 0;
	USART1->BRR = USART_BRR(UART_CLK, UART_BAUD);
	USART1->CR1 |= USART_CR1_UE;
}

static inline void writeChar(char c)
{
	while (!(USART1->SR & USART_SR_TXE));
	USART1->DR = c;
}

void usart1WriteChar(char c)
{
	writeChar(c);
}

void usart1WriteString(const char *str)
{
	while (*str != '\0') {
		if (*str == '\n')
			writeChar('\r');
		writeChar(*str++);
	}
}

void usart1DumpHex(uint32_t v)
{
	writeChar('0');
	writeChar('x');
	uint32_t zero = 1;
	for (uint32_t i = 8; i != 0; i--) {
		uint8_t c = v >> 28;
		if (!zero || i == 1 || c != 0) {
			zero = 0;
			if (c < 0xa)
				c += '0';
			else
				c += 'a' - 0xa;
			writeChar(c);
		}
		v <<= 4;
	}
}
