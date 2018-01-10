#include <stm32f7xx.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <macros.h>
#include <debug.h>
#include <escape.h>
#include <system/pvd.h>
#include "flash.h"

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

SECTION(.iram) STATIC_INLINE void flash_wait()
{
	__DSB();
	while (FLASH->SR & FLASH_SR_BSY_Msk);
}

SECTION(.iram) STATIC_INLINE int flash_unlock()
{
	flash_wait();
	if (!(FLASH->CR & FLASH_CR_LOCK_Msk))
		return 1;
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xcdef89ab;
	flash_wait();
	return !(FLASH->CR & FLASH_CR_LOCK_Msk);
}

SECTION(.iram) extern void flash_erase(uint32_t addr)
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
	// Program size x32
	FLASH->CR = (0b10 << FLASH_CR_PSIZE_Pos) |
			FLASH_CR_SER_Msk | (sec << FLASH_CR_SNB_Pos);
	FLASH->CR |= FLASH_CR_STRT_Msk;
	sector = sec;
}

SECTION(.iram) STATIC_INLINE void flash_write(uint32_t addr, uint8_t size, uint8_t *data)
{
	// Only AXI interface has write access
	uint32_t fsize = 1024ul * FLASH_SIZE;
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

// Use extern to force gcc place the code in RAM
SECTION(.iram) extern void flash_hex()
{
	__disable_irq();
	// Unlock flash
	if (!flash_unlock()) {
		dbgbkpt();
		goto reset;
	}
	// Process HEX content
	union {
		struct PACKED {
			uint16_t base;
			uint8_t ext[2];
		};
		uint32_t u32;
		uint8_t u8[4];
	} addr;
	for (hex_t *hp = hex; hp; hp = hp->next) {
		switch (hp->ihex.type) {
		case IData:	// Data
			addr.base = hp->ihex.addr;
			flash_write(addr.u32, hp->ihex.cnt, hp->ihex.payload);
			break;
		case IEOF:	// End Of File
			goto reset;
		case IELAddr:	// Extended Linear Address
			addr.ext[1] = hp->ihex.payload[0];
			addr.ext[0] = hp->ihex.payload[1];
			break;
		case ISLAddr:	// Start Linear Address
			addr.u8[3] = hp->ihex.payload[0];
			addr.u8[2] = hp->ihex.payload[1];
			addr.u8[1] = hp->ihex.payload[2];
			addr.u8[0] = hp->ihex.payload[3];
			// Set reset vector
			SCB->VTOR = RAMDTCM_BASE;
			*(((uint32_t *)RAMDTCM_BASE) + 1u) = addr.u32;
			break;
		default:
			__BKPT(0);
		}
	}
reset:
	flash_wait();
	FLASH->CR = FLASH_CR_LOCK_Msk;
	// gcc may not inline NVIC_SystemReset();
	__DSB();
	SCB->AIRCR = (0x5FAUL << SCB_AIRCR_VECTKEY_Pos) |
			(SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) |
			SCB_AIRCR_SYSRESETREQ_Msk;
	__DSB();
	for (;;);
}

/* Intel HEX format handling functions */

void flash_hex_free()
{
	dbgprintf(ESC_DEBUG "Resetting flash buffer...\n");
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
		dbgprintf(ESC_ERROR "Data too short\n");
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
		__BKPT(0);
		break;
	case ISSAddr:
		if (ip->cnt != 4u)
			goto bcerr;
		__BKPT(0);
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
		dbgprintf(ESC_ERROR "Invalid record type\n");
		return 0;
	}
	uint8_t cksum = 0;
	for (uint8_t *p = (uint8_t *)ip; size--; cksum += *p++);
	if (cksum != 0u) {
		dbgprintf(ESC_ERROR "Checksum failed\n");
		return 0;
	}
	return 1;
bcerr:
	dbgprintf(ESC_ERROR "Invalid byte count\n");
	return 0;
}

void flash_hex_data(uint8_t size, void *payload)
{
	if (!hex)
		dbgprintf(ESC_DEBUG "Receiving HEX content...\n");
	ihex_t *ip = payload;
	hex_invalid = hex_invalid ?: !hex_verify(size, ip);
	if (hex_invalid)
		return;
	hex_t *hp = malloc(sizeof(hex_t) + ip->cnt);
	if (!hp)
		panic();
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
	if (hex_invalid || !hex) {
		NVIC_SystemReset();
		return;
	}
	uint32_t size = 0;
	for (hex_t **hp = &hex; *hp; hp = &(*hp)->next)
		size += (*hp)->ihex.cnt;
	printf(ESC_WRITE "Flashing %lu bytes...\n", size);
	pvd_disable_all();
	flash_hex();
}
