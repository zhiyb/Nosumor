#include <stdint.h>
#include "macros.h"
#include "uart.h"

void initUART()
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
}
