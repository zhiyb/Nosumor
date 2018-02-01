#ifndef LED_H
#define LED_H

#include <stdint.h>

#if HWVER == 0x0002
#define LED_NUM	3
#else
#define LED_NUM	4
#endif

void led_init();
// Format: Position(8), Elements(4), Bits(4)
const void *led_info(uint8_t *num);

void led_set(uint32_t i, uint32_t size, const uint16_t *c);
void led_get(uint32_t i, uint32_t size, uint16_t *c);

#endif // LED_H
