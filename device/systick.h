#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void systick_init(uint32_t hz);
uint32_t systick_cnt();
void systick_delay(uint32_t cycles);

#ifdef __cplusplus
}
#endif

#endif // SYSTICK_H
