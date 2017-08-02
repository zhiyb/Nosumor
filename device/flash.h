#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_SIZE	(*(uint16_t *)0x1ff07a22)

void flash_hex_free();
void flash_hex_data(uint8_t size, void *payload);
int flash_hex_check();
void flash_hex_program();

#ifdef __cplusplus
}
#endif

#endif // FLASH_H
