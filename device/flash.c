#include <stm32f7xx.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <macros.h>
#include <debug.h>
#include <escape.h>
#include <pvd.h>
#include "flash.h"

#define FLASH_SIZE	(*(__IO uint32_t *)0x1ff07a22)

enum FlashInterface {AXI, ICTM};

// Store hex file for programming
typedef struct PACKED ihex_t {
	uint8_t cnt;
	uint16_t addr;
	uint8_t type;
	uint8_t payload[];
} ihex_t;

enum IHEXTypes {IData = 0, IEOF, IESAddr, ISSAddr, IELAddr, ISLAddr};

typedef struct hex_t {
	struct hex_t *next;
	ihex_t ihex;
} hex_t;

static hex_t *hex = 0, **hex_last = &hex;
static int hex_invalid = 0;

/* Flash access functions */

SECTION(.ram) static inline void flash_wait()
{
	__DSB();
	while (FLASH->SR & FLASH_SR_BSY_Msk);
}

SECTION(.ram) static inline int flash_unlock()
{
	flash_wait();
	if (!(FLASH->CR & FLASH_CR_LOCK_Msk))
		return 1;
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xcdef89ab;
	return !(FLASH->CR & FLASH_CR_LOCK_Msk);
}

SECTION(.ram) static void flash_erase(uint32_t addr)
{
	// Flash sector start addresses
	static uint32_t start_addr[] SECTION(.data) = {
		0x08000000, 0x08004000, 0x08008000, 0x0800c000,
		0x08010000, 0x08020000, 0x08040000, 0x08060000,
	};
	// Map to sector
	static int sector = -1;
	int sec = -1;
	for (uint32_t i = 0; i != ASIZE(start_addr); i++)
		if (addr >= start_addr[i])
			sec = i;
		else
			break;
	if (sec == -1 || sec == sector)
		return;
	// Erase sector
	flash_wait();
	// Program size x64
	FLASH->CR = (0b11 << FLASH_CR_PSIZE_Pos) |
			FLASH_CR_SER_Msk | (sec << FLASH_CR_SNB_Pos);
	FLASH->CR |= FLASH_CR_STRT_Msk;
	sector = sec;
}

SECTION(.ram) static inline void flash_write(uint32_t addr, uint8_t size, uint8_t *data)
{
	// Only AXI interface has write access
	uint32_t fsize = FLASH_SIZE;
	if (addr >= FLASHAXI_BASE && addr < FLASHAXI_BASE + fsize)
		// Access using AXI interface
		;
	else if (addr >= FLASHITCM_BASE && addr < FLASHITCM_BASE + fsize)
		// Change ITCM access to AXI interface
		addr = addr - FLASHITCM_BASE + FLASHAXI_BASE;
	else {
		dbgbkpt();
		return;
	}
	// Erase corresponding area
	flash_erase(addr);
	flash_erase(addr + size - 1u);
	flash_wait();
	// Program size x8
	FLASH->CR = (0b00 << FLASH_CR_PSIZE_Pos) | FLASH_CR_PG_Msk;
	while (size--) {
		*(uint8_t *)addr++ = *data++;
		flash_wait();
	}
}

// Use extern to force gcc place the code in .ram section
SECTION(.ram) extern void flash_hex()
{
	__disable_irq();
	// Unlock flash
	if (!flash_unlock())
		goto failed;
	// Process HEX content
	union {
		struct PACKED {
			uint16_t base;
			uint8_t ext[2];
		};
		uint32_t u32;
	} addr;
	for (hex_t *hp = hex; hp; hp = hp->next) {
		switch (hp->ihex.type) {
		case IData:	// Data
			addr.base = hp->ihex.addr;
			flash_write(addr.u32, hp->ihex.cnt, hp->ihex.payload);
			break;
		case IEOF:	// End Of File
			flash_wait();
			FLASH->CR = FLASH_CR_LOCK_Msk;
			NVIC_SystemReset();
			break;
		case IESAddr:	// Extended Segment Address
			dbgbkpt();
			break;
		case ISSAddr:	// Start Segment Address
			dbgbkpt();
			break;
		case IELAddr:	// Extended Linear Address
			addr.ext[1] = hp->ihex.payload[0];
			addr.ext[0] = hp->ihex.payload[1];
			break;
		case ISLAddr:	// Start Linear Address
			break;
		default:
			__BKPT(0);
		}
	}
failed:
	dbgbkpt();
	NVIC_SystemReset();
}

/* Intel HEX format handling functions */

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
	if (ip->cnt != size - 5u)
		goto bcerr;
	switch (ip->type) {
	case IData:
		break;
	case IEOF:
		if (ip->cnt != 0u)
			goto bcerr;
		break;
	case IESAddr:
		if (ip->cnt != 2u)
			goto bcerr;
		dbgbkpt();
		break;
	case ISSAddr:
		if (ip->cnt != 4u)
			goto bcerr;
		dbgbkpt();
		break;
	case IELAddr:
		if (ip->cnt != 2u)
			goto bcerr;
		break;
	case ISLAddr:
		if (ip->cnt != 4u)
			goto bcerr;
		break;
	default:
		dbgprintf(ESC_RED "Invalid record type\n");
		return 0;
	}
	uint8_t cksum = 0;
	for (uint8_t *p = (uint8_t *)ip; size--; cksum += *p++);
	if (cksum != 0u) {
		dbgprintf(ESC_RED "Checksum failed\n");
		return 0;
	}
	return 1;
bcerr:
	dbgprintf(ESC_RED "Invalid byte count\n");
	return 0;
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
	ip->addr = __REV16(ip->addr);		// Endianness conversion
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
	flash_hex();
}
