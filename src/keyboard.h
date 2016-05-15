#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include "stm32f1xx.h"
#include "macros.h"

// Port A, lower byte
#define LED_GPIO  GPIOA
#define LED_LEFT  1
#define LED_RIGHT 2
#define LED_BLUE  3
#define LED_GREEN 4
#define LED_RED   5

// Port B, higher byte
#define KEY_GPIO  GPIOB
#define KEY_LEFT  9
#define KEY_RIGHT 10
#define KEY_1     13
#define KEY_2     14
#define KEY_3     15

void initKeyboard();

static inline void setLED(uint32_t led, uint32_t val)
{
	if (val)
		LED_GPIO->BSRR = BV(led);
	else
		LED_GPIO->BRR = BV(led);
}

static inline void toggleLED(uint32_t led)
{
	LED_GPIO->ODR ^= BV(led);
}

static inline uint32_t readKey(uint32_t key)
{
	return !(KEY_GPIO->IDR & BV(key));
}

#endif // KEYBOARD_H
