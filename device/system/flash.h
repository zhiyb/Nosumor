#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>
#include <stm32f722xx.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_SIZE	(*(uint16_t *)FLASHSIZE_BASE)

void flash_hex_free();
void flash_hex_data(uint8_t size, void *payload);
int flash_hex_check();
void flash_hex_program();
void flash_fatfs_hex_program(const char *path);
void *flash_buffer(uint32_t *length);

#ifdef __cplusplus
}
#endif

#endif // FLASH_H
