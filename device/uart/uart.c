#include <device.h>
#include <clocks.h>
#include <module.h>
#include <fio.h>

static void init(USART_TypeDef *uart)
{
	switch ((uint32_t)uart) {
	case (uint32_t)USART6:
		// Enable USART6 clock
		RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
		// Configure GPIOs
		RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
		// 10: Alternative function mode
		GPIO_MODER(GPIOC, 6, GPIO_MODER_ALTERNATE);
		GPIO_MODER(GPIOC, 7, GPIO_MODER_ALTERNATE);
		GPIO_OTYPER_PP(GPIOC, 6);
		GPIO_OSPEEDR(GPIOC, 6, GPIO_OSPEEDR_LOW);
		GPIO_PUPDR(GPIOC, 7, GPIO_PUPDR_UP);
		// AF8: USART6
		GPIO_AFRL(GPIOC, 6, 8);
		GPIO_AFRL(GPIOC, 7, 8);
		break;
	}
}

static void uart_config(USART_TypeDef *uart, uint32_t baud)
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

static void uart_putc(void *base, char c)
{
	USART_TypeDef *uart = base;
	while (!(uart->ISR & USART_ISR_TXE));
	uart->TDR = c;
}

static char uart_getc(void *base)
{
	USART_TypeDef *uart = base;
	while (!(uart->ISR & USART_ISR_RXNE));
	return uart->RDR;
}

static void *handler(void *inst, uint32_t msg, void *data)
{
	UNUSED(data);
	USART_TypeDef *pinst = inst;
	if (msg == HASH("init")) {
		init(pinst);
		return 0;
	} else if (msg == HASH("config")) {
		uart_config(pinst, (uint32_t)data);
		return data;
	} else if (msg == HASH("stdio")) {
		fio_setup(uart_putc, uart_getc, inst);
		return 0;
	}
	return 0;
}

MODULE("uart", 0, USART6, handler);
