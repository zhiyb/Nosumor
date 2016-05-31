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

#ifdef __cplusplus
extern "C" {
#endif

void initKeyboard();

static inline void setLED(uint16_t led, uint16_t val)
{
	if (val)
		LED_GPIO->BSRR = BV(led);
	else
		LED_GPIO->BRR = BV(led);
}

static inline void toggleLED(uint16_t led)
{
	LED_GPIO->ODR ^= BV(led);
}

#if 0
static inline uint32_t readKey(uint16_t key)
{
	return !(KEY_GPIO->IDR & BV(key));
}
#else
uint32_t readKey(uint16_t key);
#endif

#ifdef __cplusplus
}
#endif

#endif // KEYBOARD_H
