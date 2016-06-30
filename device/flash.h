#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void flashInit();
void flashSRAM();
uint32_t flashBusy();

#ifdef __cplusplus
}
#endif

#endif // FLASH_H
