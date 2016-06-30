#include <stdint.h>
#include "stm32f1xx.h"
#include "keyboard.h"
#include "usb.h"
#include "usb_desc.h"
#include "usb_class.h"
#include "usart1.h"
#include "flash.h"
#include "escape.h"
#include "debug.h"

#ifdef DEBUG
#define VARIANT	"DEBUG"
#else
#define VARIANT	"RELEASE"
#endif

void rccInit()
{
	// Enable external crystal oscillator
	RCC->CR |= RCC_CR_HSEON;
	while (!(RCC->CR & RCC_CR_HSERDY));
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSE;
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE);

	// Enable PLL
	// Clock from 8MHz HSE, x9 to 72MHz system clock
	// System timer	@ 72 MHz
	// APB1		@ 36 MHz
	// APB2		@ 72 MHz
	// USB		@ 48 MHz
	RCC->CFGR = RCC_CFGR_PLLMULL9 | RCC_CFGR_PLLSRC |
			RCC_CFGR_PPRE1_DIV2 /*| RCC_CFGR_PPRE2_DIV4*/ |
			RCC_CFGR_SW_HSE;
	RCC->CR = RCC_CR_HSION | RCC_CR_HSEON | RCC_CR_PLLON;
	while (!(RCC->CR & RCC_CR_PLLRDY));
	// 2 flash wait states for SYSCLK >= 48 MHz
	FLASH->ACR = FLASH_ACR_PRFTBS | FLASH_ACR_PRFTBE | 2;
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

void init()
{
	// Vector Table Relocation in Internal SRAM
	SCB->VTOR = SRAM_BASE;

	rccInit();
	NVIC_SetPriorityGrouping(5);	// 2+2 bits pripority
	// Enable DMA
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
#if 0
	// Enable SWJ only
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
#endif

	usart1Init();
	usart1WriteString("\n" ESC_CYAN VARIANT " build @ " __DATE__ " " __TIME__ ESC_DEFAULT "\n");
	flashInit();
	usbInit();
	keyboardInit();
	__enable_irq();
}

int main()
{
	init();
	const uint16_t keyCheck = BV(KEY_1);
	if (dbexist() && readKeys(keyCheck) == keyCheck)
		flashSRAM();

	for (;;) {
		setLED(LED_LEFT, readKey(KEY_LEFT));
		setLED(LED_RIGHT, readKey(KEY_RIGHT));

		if (dbexist()) {
			setLED(LED_RED, flashBusy());
			setLED(LED_GREEN, readKey(KEY_2));
			setLED(LED_BLUE, readKey(KEY_3));
		} else {
			setLED(LED_RED, readKey(KEY_1));
			setLED(LED_GREEN, readKey(KEY_2));
			setLED(LED_BLUE, readKey(KEY_3));
		}
	}
}
