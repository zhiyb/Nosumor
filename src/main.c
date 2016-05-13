#include "stm32f1xx.h"
#include "keyboard.h"
#include "debug.h"

void initRCC()
{
	// Enable external crystal oscillator
	RCC->CR |= RCC_CR_HSEON;
	while (!(RCC->CR & RCC_CR_HSERDY));
	RCC->CFGR |= RCC_CFGR_SW_HSE;
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE);

	// Disable internal oscillator, enable PLL
	// Clock from 8MHz HSE, x9 to 72MHz system clock
	// System timer	@ 72 MHz
	// APB1		@ 36 MHz
	// APB2		@ 72 MHz
	// USB		@ 48 MHz
	RCC->CFGR = RCC_CFGR_PLLMULL9 | RCC_CFGR_PLLSRC | RCC_CFGR_PPRE1_DIV2;
	RCC->CR = RCC_CR_HSEON | RCC_CR_PLLON;
	while (!(RCC->CR & RCC_CR_PLLRDY));
	RCC->CFGR |= RCC_CFGR_SW_PLL;
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

void initGPIO()
{
	initKeyboard();
}

void init()
{
	initRCC();
	SystemCoreClockUpdate();
	initGPIO();
}

int main()
{
	init();

	for (;;) {
		setLED(LED_LEFT, readKey(KEY_LEFT));
		setLED(LED_RIGHT, readKey(KEY_RIGHT));
		setLED(LED_RED, readKey(KEY_1));
		setLED(LED_GREEN, readKey(KEY_2));
		setLED(LED_BLUE, readKey(KEY_3));
	}
}
