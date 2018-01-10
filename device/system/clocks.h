#ifndef CLOCKS_H
#define CLOCKS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t clkAHB();
uint32_t clkAPB1();
uint32_t clkAPB2();
uint32_t clkSDMMC1();
uint32_t clkSDMMC2();
uint32_t clkTimer(uint32_t i);

void rcc_init();

#ifdef __cplusplus
}
#endif

#endif // CLOCKS_H
