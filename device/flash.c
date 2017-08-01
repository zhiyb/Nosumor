#include <stm32f7xx.h>
#include <malloc.h>
#include <macros.h>
#include <debug.h>
#include <escape.h>
#include <pvd.h>
#include "flash.h"

// Store hex file for programming
typedef struct PACKED ihex_t {
	uint8_t cnt;
	uint16_t addr;
	uint8_t type;
	uint8_t payload[];
} ihex_t;

typedef struct hex_t {
	struct hex_t *next;
	ihex_t ihex;
} hex_t;

static hex_t *hex = 0, **hex_last = &hex;
static int hex_invalid = 0;

void flash_hex_free()
{
	dbgprintf(ESC_CYAN "Resetting flash buffer...\n");
	hex_t *next;
	for (hex_t *hp = hex; hp; hp = next) {
		next = hp->next;
		free(hp);
	}
	hex = 0;
	hex_last = &hex;
	hex_invalid = 0;
}

static int hex_verify(uint8_t size, ihex_t *ip)
{
	if (size < 5u) {
		dbgprintf(ESC_RED "Data too short\n");
		return 0;
	}
	if (ip->cnt != size - 5u) {
		dbgprintf(ESC_RED "Invalid byte count\n");
		return 0;
	}
	uint8_t cksum = 0;
	for (uint8_t *p = (uint8_t *)ip; size--; cksum += *p++);
	if (cksum != 0u) {
		dbgprintf(ESC_RED "Checksum failed\n");
		return 0;
	}
	return 1;
}

void flash_hex_data(uint8_t size, void *payload)
{
	if (!hex)
		dbgprintf(ESC_CYAN "Receiving HEX content...\n");
	ihex_t *ip = payload;
	hex_invalid = hex_invalid ?: !hex_verify(size, ip);
	if (hex_invalid)
		return;
	hex_t *hp = malloc(sizeof(hex_t) + ip->cnt);
	memcpy(&hp->ihex, ip, size - 1u);	// Remove checksum
	hp->next = 0;
	*hex_last = hp;
	hex_last = &hp->next;
}

int flash_hex_check()
{
	return !hex_invalid;
}

void flash_hex_program()
{
	if (hex_invalid) {
		dbgbkpt();
		NVIC_SystemReset();
		return;
	}
	uint32_t size = 0;
	for (hex_t **hp = &hex; *hp; hp = &(*hp)->next)
		size += (*hp)->ihex.cnt;
	printf(ESC_CYAN "Flashing %lu bytes...\n", size);
	pvd_disable_all();
	dbgbkpt();
}
