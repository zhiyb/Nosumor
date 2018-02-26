#include <stm32f7xx.h>
#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <macros.h>
#include <debug.h>
#include <escape.h>
#include <system/pvd.h>
#include <peripheral/led.h>
#include <logic/scsi.h>
#include <fatfs/ff.h>
#include "flash.h"

#ifdef BOOTLOADER
#define BUFFER_SIZE	(128 * 1024)
#else
#define BUFFER_SIZE	(16 * 1024)
#endif

extern uint32_t __app_start__, __appi_start__;

enum FlashInterface {AXI, ICTM};

// Store hex file for programming
typedef struct PACKED ihex_t {
	uint8_t cnt;
	uint16_t addr;
	uint8_t type;
	uint8_t payload[];
} ihex_t;

enum IHEXTypes {IData = 0, IEOF, IESAddr, ISSAddr, IELAddr, ISLAddr};

typedef struct PACKED hex_seg_t {
	uint32_t cnt;
	uint32_t addr;
	uint8_t payload[];
} hex_seg_t;

typedef struct hex_t {
	struct hex_t *next;
	ihex_t ihex;
} hex_t;

static hex_t *hex = 0, **hex_last = &hex;
static int hex_invalid = 0;
static uint32_t seg, saddr;

static uint8_t flash_buf[BUFFER_SIZE] ALIGN(32);

void *flash_buffer(uint32_t *length)
{
	if (length)
		*length = sizeof(flash_buf);
	return flash_buf;
}

/* Flash access functions */

SECTION(.iram) extern void flushCache()
{
	SCB_CleanInvalidateDCache();
}

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

SECTION(.iram) STATIC_INLINE void flash_write(uint32_t addr, uint32_t size, uint8_t *data)
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

#ifndef BOOTLOADER
// Use extern to force gcc place the code in RAM
SECTION(.iram) extern void flash_hex()
{
	__disable_irq();
	SCB_CleanInvalidateDCache();
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
	for (;;)
		__NOP();
}
#endif

// Use extern to force gcc place the code in RAM
SECTION(.iram) extern void flash_hex_segments(hex_seg_t *seg)
{
	__disable_irq();
	SCB_CleanInvalidateDCache();
	// Unlock flash
	if (!flash_unlock()) {
		dbgbkpt();
		goto reset;
	}
	// Flash segments
	while (seg->cnt) {
		flash_write(seg->addr, seg->cnt, seg->payload);
		seg = (void *)(seg->payload + seg->cnt);
	}
	// Done
	if (saddr >= (uint32_t)&__appi_start__) {
		flash_wait();
		// Program size x32
		FLASH->CR = (0b10 << FLASH_CR_PSIZE_Pos) | FLASH_CR_PG_Msk;
		__app_start__ = 0;
	}
reset:	flash_wait();
	FLASH->CR = FLASH_CR_LOCK_Msk;
	// Soft reset
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
	dbgprintf(ESC_DEBUG "[HEX] Resetting flash buffer...\n");
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
		dbgprintf(ESC_ERROR "[HEX] Data too short\n");
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
		return 0;
	case ISSAddr:
		if (ip->cnt != 4u)
			goto bcerr;
		dbgbkpt();
		return 0;
	case IELAddr:
		if (ip->cnt != 2u)
			goto bcerr;
		break;
	case ISLAddr:
		if (ip->cnt != 4u)
			goto bcerr;
		break;
	default:
		dbgprintf(ESC_ERROR "[HEX] Invalid record type\n");
		return 0;
	}
	uint8_t cksum = 0;
	for (uint8_t *p = (uint8_t *)ip; size--; cksum += *p++);
	if (cksum != 0u) {
		dbgprintf(ESC_ERROR "[HEX] Checksum failed\n");
		return 0;
	}
	return 1;
bcerr:
	dbgprintf(ESC_ERROR "[HEX] Invalid byte count\n");
	return 0;
}

void flash_hex_data(uint8_t size, void *payload)
{
	if (!hex)
		dbgprintf(ESC_DEBUG "[HEX] Receiving HEX content...\n");
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

#ifndef BOOTLOADER
void flash_hex_program()
{
	if (hex_invalid || !hex) {
		NVIC_SystemReset();
		return;
	}
	uint32_t size = 0;
	for (hex_t **hp = &hex; *hp; hp = &(*hp)->next)
		size += (*hp)->ihex.cnt;
	printf(ESC_WRITE "[HEX] Flashing %lu bytes...\n", size);
	pvd_disable_all();
	flash_hex();
}
#endif

static uint32_t toUnsigned(char *s, uint8_t b)
{
	uint32_t v = 0;
	while (b--) {
		unsigned int i;
		char c = tolower(*s++);
		if (c >= '0' && c <= '9')
			i = c - '0';
		else
			i = c - 'a' + 0x0a;
		v <<= 4;
		v |= i;
	}
	return v;
}

static int fatfs_hex_parse(FIL *fp, uint8_t *buf, uint32_t *addr)
{
	FRESULT res;
	UINT s;
	uint32_t i;
	uint8_t ck = 0;
	char cbuf[8];
	// Read line header
	if ((res = f_read(fp, cbuf, 8, &s)) != FR_OK) {
		dbgprintf(ESC_ERROR "[HEX] Failure reading file: "
			  ESC_DATA "%u\n", res);
		return -1;
	}
	if (s != 8) {
		dbgprintf(ESC_ERROR "[HEX] Unexpected end of file\n");
		return -1;
	}

	// Parse line header
	uint8_t *up;
	ihex_t ihex;
	ihex.cnt = toUnsigned(&cbuf[0], 2);
	ihex.addr = toUnsigned(&cbuf[2], 4);
	ihex.type = toUnsigned(&cbuf[6], 2);
	for (up = (void *)&ihex, i = 4; i--; ck += *up++);

	// Parse data
	i = ((uint32_t)ihex.cnt << 1) + 2;
	char dbuf[i], *cp = dbuf;
	if ((res = f_read(fp, dbuf, i, &s)) != FR_OK) {
		dbgprintf(ESC_ERROR "[HEX] Failure reading file: "
			  ESC_DATA "%u\n", res);
		return -1;
	}
	if (s != i) {
		dbgprintf(ESC_ERROR "[HEX] Unexpected end of file\n");
		return -1;
	}
	for (cp = dbuf, i = ihex.cnt; i--; cp += 2)
		ck += *buf++ = toUnsigned(cp, 2);

	// Parse checksum
	ck += toUnsigned(cp, 2);
	if (ck != 0) {
		dbgprintf(ESC_ERROR "[HEX] Checksum failed\n");
		return -1;
	}

	switch (ihex.type) {
	case IData:
		*addr = seg | (uint32_t)ihex.addr;
		return ihex.cnt;
	case IEOF:
		if (ihex.cnt != 0u)
			break;
		return 0;
	case IESAddr:
		if (ihex.cnt != 2u)
			break;
		dbgbkpt();
		break;
	case ISSAddr:
		if (ihex.cnt != 4u)
			break;
		dbgbkpt();
		break;
	case IELAddr:
		if (ihex.cnt != 2u)
			break;
		seg = 0;
		memcpy((void *)&seg + 2, buf - 2, 2);
		seg = __REV16(seg);
		return 0;
	case ISLAddr:
		if (ihex.cnt != 4u)
			break;
		memcpy((void *)&saddr, buf - 4, 4);
		saddr = __REV(saddr);
		return 0;
	}
	dbgprintf(ESC_ERROR "[HEX] Invalid record type\n");
	return -1;
}

void flash_fatfs_hex_program(const char *path)
{
	FIL fil;
	FRESULT res;
	if ((res = f_open(&fil, path, FA_READ)) != FR_OK) {
		dbgprintf(ESC_ERROR "[HEX] Failure opening file: "
			  ESC_DATA "%u\n", res);
		return;
	}

	// Borrow buffer space
	uint64_t *buf = flash_buffer(0);
	hex_seg_t *seg = (void *)buf;
	uint8_t *p = seg->payload;
	seg->cnt = 0;

	// Parse intel HEX data
	uint32_t addr = 0;
	UINT s;
	int i;
	char c;
	// Read start code / line terminators
start:	if ((res = f_read(&fil, &c, 1, &s)) != FR_OK) {
		dbgprintf(ESC_ERROR "[HEX] Failure reading file: "
			  ESC_DATA "%u\n", res);
		return;
	}

	if (s != 1)
		goto flash;

	switch (c) {
	case ':':
		i = fatfs_hex_parse(&fil, p, &addr);
		if (i < 0)
			return;
		if (seg->cnt == 0) {
			seg->addr = addr;
			seg->cnt = i;
			p = seg->payload + i;
		} else if (seg->addr + seg->cnt != addr && i != 0) {
			// Begin new segment
			seg = (void *)(seg->payload + seg->cnt);
			memmove(seg->payload, seg, i);
			seg->addr = addr;
			seg->cnt = i;
			p = seg->payload + i;
		} else {
			seg->cnt += i;
			p += i;
		}
	case '\r':
	case '\n':
		goto start;
	default:
		dbgprintf(ESC_ERROR "[HEX] Invalid file format\n");
		return;
	}

	// Start flashing
flash:	// Ending segment
	seg = (void *)(seg->payload + seg->cnt);
	seg->cnt = 0;

	for (uint8_t i = LED_NUM; i--;)
		led_set(i, 3, (const uint16_t[3]){0, 0x03ff, 0});

#ifdef DEBUG
	seg = (void *)buf;
	while (seg->cnt) {
		printf(ESC_INFO "[HEX] Segment " ESC_DATA "0x%08lx"
		       ESC_INFO " has " ESC_DATA "%lu" ESC_INFO " bytes\n",
		       seg->addr, seg->cnt);
		seg = (void *)(seg->payload + seg->cnt);
	}
#endif

	pvd_disable_all();
	flash_hex_segments((void *)buf);
}
