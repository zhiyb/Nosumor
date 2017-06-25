#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t clkAHB();
uint32_t clkAPB1();
uint32_t clkAPB2();

void rcc_init();

#ifdef __cplusplus
}
#endif

#endif // CLOCK_H
