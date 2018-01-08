#ifndef RGB_H
#define RGB_H

#include <stdint.h>

#define LED_NUM	4

void led_init();
// Format: Position(8), Elements(4), Bits(4)
const void *led_info(uint8_t *num);

void led_set(uint32_t i, uint32_t size, const uint16_t *c);
void led_get(uint32_t i, uint32_t size, uint16_t *c);

#endif // RGB_H
