#include <stdint.h>
#include "stm32f1xx.h"
#include "keyboard.h"
#include "usb.h"
#include "usb_desc.h"
#include "usb_class.h"
#include "usart1.h"
#include "escape.h"
#include "debug.h"

#ifdef DEBUG
#define VARIANT	"DEBUG"
#else
#define VARIANT	"RELEASE"
#endif

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
	SystemCoreClockUpdate();
}

void init()
{
	initRCC();
	NVIC_SetPriorityGrouping(4);	// 3+1 bits pripority
	// Enable DMA
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;

	initUSART1();
	initUSB();
	initKeyboard();
	// Enable SWJ only
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
	usart1WriteString("\n" ESC_CYAN VARIANT " build @ " __DATE__ " " __TIME__ ESC_DEFAULT "\n");
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

		if (readKey(KEY_LEFT)) {
			struct mouse_t mouse = {HID_MOUSE, 0x00, -1, -1, 0};
			usbHIDReport(&mouse, sizeof(mouse));
			//while (readKey(KEY_LEFT));
		}

		if (readKey(KEY_RIGHT)) {
			struct mouse_t mouse = {HID_MOUSE, 0x00, 1, 1, 0};
			usbHIDReport(&mouse, sizeof(mouse));
			//while (readKey(KEY_RIGHT));
		}

		if (readKey(KEY_3)) {
			//dbbkpt();
		}
	}
}
