#include <stm32f7xx.h>
#include "../macros.h"
#include "audio.h"

void audio_init()
{
	// Enable I2C
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN_Msk;
}
