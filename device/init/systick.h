#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>
#include <list.h>

typedef void (* const systick_callback_t)(uint32_t cnt);

#define SYSTICK_CALLBACK(func)	LIST_ITEM(systick, systick_callback_t) = (func)

void systick_init(uint32_t hz);
void systick_enable(uint32_t e);
uint32_t systick_cnt();
void systick_delay(uint32_t cycles);

#endif // SYSTICK_H
