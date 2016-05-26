#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "stm32f1xx.h"

// Port A, lower byte
#define UART_GPIO	GPIOA
#define UART_TX		9
#define UART_RX		10
#define UART		USART1

void initUART();

#endif // UART_H
