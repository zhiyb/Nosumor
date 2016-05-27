#ifndef USART1_H
#define USART1_H

#include <stdint.h>
#include "stm32f1xx.h"

void initUSART1();
void usart1WriteChar(char c);
void usart1WriteString(char *str);
void usart1DumpHex(uint32_t v);

#endif // USART1_H
