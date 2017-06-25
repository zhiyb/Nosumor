#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stm32f7xx.h>

#ifdef __cplusplus
extern "C" {
#endif

void uart_init(USART_TypeDef *uart);
void uart_config(USART_TypeDef *uart, uint32_t baud);

void uart_putc(void *base, char c);
char uart_getc(void *base);

#ifdef __cplusplus
}
#endif

#endif // UART_H
