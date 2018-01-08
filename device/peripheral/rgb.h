#ifndef RGB_H
#define RGB_H

#include <stdint.h>

#define RGB_NUM	4

void rgb_init();
// Format: Position(8), Elements(4), Bits(4)
const void *rgb_info(uint8_t *num);

void rgb_set(uint32_t i, uint32_t size, const uint16_t *c);
void rgb_get(uint32_t i, uint32_t size, uint16_t *c);

#endif // RGB_H
