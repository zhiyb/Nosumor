#include "stm32f1xx.h"
#include "flash.h"
#include "usart1.h"
#include "escape.h"

//#undef DEBUG
#ifdef DEBUG
#define writeChar(c)		usart1WriteChar((c))
#define writeString(str)	usart1WriteString((str))
#define dumpHex(v)		usart1DumpHex((uint32_t)(v))
#else	// DEBUG
#define writeChar(c)		((void)0)
#define writeString(str)	((void)0)
#define dumpHex(v)		((void)0)
#endif	// DEBUG

#define FLASH_CR_DEFAULT	(FLASH_CR_EOPIE | FLASH_CR_OPTWRE)

extern uint32_t _sflash_sram;
extern uint32_t _eflash_sram;
extern uint32_t _edata;
static const uint32_t page_size = 1024U;

enum Mode {Standby = 0, PageErase = 0x01, Programming = 0x02};

static struct {
	enum Mode mode;
	uint32_t pageCount;
	uint16_t *dest, *src;
	uint32_t size;
} status;

void flashInit()
{
	uint32_t prioritygroup = NVIC_GetPriorityGrouping();
	NVIC_SetPriority(FLASH_IRQn, NVIC_EncodePriority(prioritygroup, 3, 3));
	NVIC_EnableIRQ(FLASH_IRQn);
	status.mode = Standby;
}

uint32_t flashBusy()
{
	return FLASH->SR & FLASH_SR_BSY || status.mode != Standby;
}

static void fpecUnlock()
{
	if (FLASH->CR & FLASH_CR_LOCK) {
		writeString(ESC_RED "FLASH: Unlocking" ESC_DEFAULT "\n");
		do {
			FLASH->KEYR = FLASH_KEY1;
			FLASH->KEYR = FLASH_KEY2;
		} while (FLASH->CR & FLASH_CR_LOCK);
		writeString(ESC_RED "FLASH: Unlocked" ESC_DEFAULT "\n");
	}
}

static void flashErasePages(void *addr, uint32_t pageCount)
{
	fpecUnlock();
	writeString(ESC_RED "FLASH: Erasing 0x");
	dumpHex(pageCount);
	writeString(" pages from 0x");
	dumpHex(addr);
	writeString(ESC_DEFAULT "\n");
	status.pageCount = pageCount;
	status.mode |= PageErase;
	FLASH->CR = FLASH_CR_DEFAULT | FLASH_CR_PER;
	FLASH->AR = (uint32_t)addr;
	FLASH->CR |= FLASH_CR_STRT;
}

static inline void flashEraseRegion(void *addr, uint32_t size)
{
	writeString(ESC_RED "FLASH: Erasing region including 0x");
	dumpHex(addr);
	writeString(" extending at least 0x");
	dumpHex(size);
	writeString(" bytes" ESC_DEFAULT "\n");
	size += (uint32_t)addr % page_size;
	addr -= (uint32_t)addr % page_size;
	flashErasePages(addr, (size + page_size - 1U) / page_size);
}

static inline void flashProgramming(void *dest, void *src, uint32_t size)
{
	writeString(ESC_RED "FLASH: Scheduled programming from 0x");
	dumpHex(src);
	writeString(" to 0x");
	dumpHex(dest);
	writeString(" of size 0x");
	dumpHex(size);
	writeString(ESC_DEFAULT "\n");
	status.dest = dest;
	status.src = src;
	status.size = (size + 1) / 2;

	uint32_t start = 1;
	__disable_irq();
	if (status.mode != Standby)
		start = 0;
	status.mode |= Programming;
	__enable_irq();

	if (start && status.size--) {
		fpecUnlock();
		FLASH->CR = FLASH_CR_DEFAULT | FLASH_CR_PG;
		*status.dest++ = *status.src++;
	}
}

void flashSRAM()
{
	writeString(ESC_RED "FLASH: SRAM started" ESC_DEFAULT "\n");
	if (flashBusy()) {
		writeString(ESC_RED "FLASH: Busy, operation cancelled" ESC_DEFAULT "\n");
		return;
	}
	flashEraseRegion(&_sflash_sram, (uint32_t)&_edata - SRAM_BASE);
	flashProgramming(&_sflash_sram, (void *)SRAM_BASE, (uint32_t)&_edata - SRAM_BASE);
}

void FLASH_IRQHandler()
{
	FLASH->SR = FLASH_SR_EOP;
	if (status.mode & PageErase) {
		if (--status.pageCount) {
			FLASH->AR += page_size;
			FLASH->CR |= FLASH_CR_STRT;
			return;
		} else {
			status.mode &= ~PageErase;
			FLASH->CR = FLASH_CR_DEFAULT | FLASH_CR_PG;
		}
	}
	if (status.mode & Programming) {
		if (status.size--) {
			*status.dest++ = *status.src++;
			return;
		} else
			status.mode &= ~Programming;
	}
}
