#include <stdint.h>
#include "keyboard.h"

void initKeyboard()
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;
	HL(LED_GPIO->CRL, LED_LEFT) &= ~(0x0c << (MOD8(LED_LEFT) << 2) |
					 0x0c << (MOD8(LED_RIGHT) << 2) |
					 0x0c << (MOD8(LED_RED) << 2) |
					 0x0c << (MOD8(LED_GREEN) << 2) |
					 0x0c << (MOD8(LED_BLUE) << 2));
	HL(LED_GPIO->CRL, LED_LEFT) |= (0x01 << (MOD8(LED_LEFT) << 2) |
					0x01 << (MOD8(LED_RIGHT) << 2) |
					0x01 << (MOD8(LED_RED) << 2) |
					0x01 << (MOD8(LED_GREEN) << 2) |
					0x01 << (MOD8(LED_BLUE) << 2));
	LED_GPIO->BRR = (BV(LED_LEFT) | BV(LED_RIGHT) |
			 BV(LED_RED) | BV(LED_GREEN) | BV(LED_BLUE));
	HL(KEY_GPIO->CRL, KEY_LEFT) &= ~(0x07 << (MOD8(KEY_LEFT) << 2) |
					 0x07 << (MOD8(KEY_RIGHT) << 2) |
					 0x07 << (MOD8(KEY_1) << 2) |
					 0x07 << (MOD8(KEY_2) << 2) |
					 0x07 << (MOD8(KEY_3) << 2));
	HL(KEY_GPIO->CRL, KEY_LEFT) |= (0x08 << (MOD8(KEY_LEFT) << 2) |
					0x08 << (MOD8(KEY_RIGHT) << 2) |
					0x08 << (MOD8(KEY_1) << 2) |
					0x08 << (MOD8(KEY_2) << 2) |
					0x08 << (MOD8(KEY_3) << 2));
	KEY_GPIO->BSRR = (BV(KEY_LEFT) | BV(KEY_RIGHT) |
			  BV(KEY_1) | BV(KEY_2) | BV(KEY_3));
}
