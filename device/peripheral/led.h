#ifndef LED_H
#define LED_H

#include <stdint.h>

#if HWVER == 0x0002
#define LED_NUM	3
#else
#define LED_NUM	4
#endif

void led_init();
void led_set(uint32_t i, const uint16_t *c);

#endif // LED_H
