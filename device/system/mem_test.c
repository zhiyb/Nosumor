#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <debug.h>
#include <macros.h>
#include <device.h>
#include <system/clocks.h>
#include <system/systick.h>

#define MEM_SIZE	(12 * 1024)
#define MEM_REPEAT	4

#define ACCESS_R	1
#define ACCESS_W	2

extern uint8_t __itcm_start__;
extern uint8_t __itcm_end__;
static const uint32_t itcm_size = 16 * 1024;

static struct {
	const char *name;
	uint32_t addr;
	uint32_t access;
} region[] = {
	{"ITCM",	RAMITCM_BASE + 4 * 1024,	ACCESS_R | ACCESS_W},
	{"DTCM",	RAMDTCM_BASE,			ACCESS_R | ACCESS_W},
	{"SRAM1",	SRAM1_BASE,			ACCESS_R | ACCESS_W},
	{"SRAM2",	SRAM2_BASE,			ACCESS_R | ACCESS_W},
	{"FLASH-AXIM",	FLASHAXI_BASE,			ACCESS_R},
	{"FLASH-ITCM",	FLASHITCM_BASE,			ACCESS_R},
};

enum {RegionITCM = 0, RegionDTCM, RegionSRAM1, RegionSRAM2, RegionFAXIM, RegionFITCM};

static int32_t copy_memcpy(void *dst, void *src, uint32_t size);
static int32_t copy_memcpy_flush(void *dst, void *src, uint32_t size);
static int32_t copy_dmacpy(void *dst, void *src, uint32_t size);
static int32_t copy_uint64cpy(void *dst, void *src, uint32_t size);
static int32_t copy_uint32cpy(void *dst, void *src, uint32_t size);

static struct {
	const char *name;
	int32_t (*func)(void *dst, void *src, uint32_t size);
} copy[] = {
	{"memcpy",	&copy_memcpy},
	{"mcpy_flush",	&copy_memcpy_flush},
	{"dma",		&copy_dmacpy},
	{"uint32",	&copy_uint32cpy},
	{"uint64",	&copy_uint64cpy},
};

static struct {
	struct {
		uint32_t freq;
		uint32_t reload;
	} stick;
} data;

register char *stack_ptr asm("sp");

SECTION(".itcm") static int32_t copy_memcpy(void *dst, void *src, uint32_t size)
{
	// Backup dst data before overwrite
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	uint8_t buf[size];
	memcpy(buf, dst, size);

	// Test start
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	uint32_t start = SysTick->VAL;
	for (int i = 0; i < MEM_REPEAT; i++) {
		memcpy(dst, src, size);
	}
	uint32_t stop = SysTick->VAL;

	// Restore dst data
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	memcpy(dst, buf, size);
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	return start - stop;
}

SECTION(".itcm") static int32_t copy_memcpy_flush(void *dst, void *src, uint32_t size)
{
	// Backup dst data before overwrite
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	uint8_t buf[size];
	memcpy(buf, dst, size);

	// Test start
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	uint32_t start = SysTick->VAL;
	for (int i = 0; i < MEM_REPEAT; i++) {
		memcpy(dst, src, size);
		SCB_CleanInvalidateDCache();
	}
	uint32_t stop = SysTick->VAL;

	// Restore dst data
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	memcpy(dst, buf, size);
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	return start - stop;
}

SECTION(".itcm") static int32_t copy_dmacpy(void *dst, void *src, uint32_t size)
{
	// DMA cannot access ITCM interface
	if ((uint32_t)src == region[RegionITCM].addr || (uint32_t)src == region[RegionFITCM].addr)
		return 0;
	if ((uint32_t)dst == region[RegionITCM].addr || (uint32_t)dst == region[RegionFITCM].addr)
		return 0;

	// Backup dst data before overwrite
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	uint8_t buf[size];
	memcpy(buf, dst, size);

	// Initialise DMA2, stream 0, channel 0
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN_Msk;
	// Clear DMA flags
	DMA2->LIFCR = DMA_LIFCR_CTCIF0_Msk | DMA_LIFCR_CHTIF0_Msk |
			DMA_LIFCR_CTEIF0_Msk | DMA_LIFCR_CDMEIF0_Msk |
			DMA_LIFCR_CFEIF0_Msk;
	// Memory to memory, 32bit -> 32bit, burst 4 beats, very high priority, transfer complete
	DMA2_Stream0->CR = (0 << DMA_SxCR_CHSEL_Pos) | (0b11 << DMA_SxCR_PL_Pos) |
			(0b01 << DMA_SxCR_MBURST_Pos) | (0b01 << DMA_SxCR_PBURST_Pos) |
			(0b10 << DMA_SxCR_MSIZE_Pos) | (0b10 << DMA_SxCR_PSIZE_Pos) |
			DMA_SxCR_MINC_Msk | DMA_SxCR_PINC_Msk | (0b10 << DMA_SxCR_DIR_Pos);
	// FIFO control, direct mode disabled, full FIFO threshold
	DMA2_Stream0->FCR = DMA_SxFCR_DMDIS_Msk | (0b11 << DMA_SxFCR_FTH_Pos);

	// Test start
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	uint32_t start = SysTick->VAL;
	for (int i = 0; i < MEM_REPEAT; i++) {
		// Peripheral address
		DMA2_Stream0->PAR = (uint32_t)src;
		// Memory address
		DMA2_Stream0->M0AR = (uint32_t)dst;
		// Number of data items
		DMA2_Stream0->NDTR = size / 4;
		// Clear DMA flags
		DMA2->LIFCR = DMA_LIFCR_CTCIF0_Msk;
		// Start DMA
		DMA2_Stream0->CR |= DMA_SxCR_EN_Msk;
		// Wait for finish
		while (!(DMA2->LISR & DMA_LISR_TCIF0_Msk));
		// Disable DMA
		DMA2_Stream0->CR &= ~DMA_SxCR_EN_Msk;
	}
	uint32_t stop = SysTick->VAL;

	// Restore dst data
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	memcpy(dst, buf, size);
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	return start - stop;
}

SECTION(".itcm") static int32_t copy_uint32cpy(void *dst, void *src, uint32_t size)
{
	// Backup dst data before overwrite
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	uint8_t buf[size];
	memcpy(buf, dst, size);

	// Test start
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	uint32_t start = SysTick->VAL;
	for (int i = 0; i < MEM_REPEAT; i++) {
		uint32_t *pdst = dst;
		uint32_t *psrc = src;
		for (uint32_t s = size / sizeof(uint32_t); s; s--)
			*pdst++ = *psrc++;
	}
	uint32_t stop = SysTick->VAL;

	// Restore dst data
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	memcpy(dst, buf, size);
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	return start - stop;
}

SECTION(".itcm") static int32_t copy_uint64cpy(void *dst, void *src, uint32_t size)
{
	// Backup dst data before overwrite
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	uint8_t buf[size];
	memcpy(buf, dst, size);

	// Test start
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	uint32_t start = SysTick->VAL;
	for (int i = 0; i < MEM_REPEAT; i++) {
		uint64_t *pdst = dst;
		uint64_t *psrc = src;
		for (uint32_t s = size / sizeof(uint64_t); s; s--)
			*pdst++ = *psrc++;
	}
	uint32_t stop = SysTick->VAL;

	// Restore dst data
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	memcpy(dst, buf, size);
	SCB_CleanInvalidateDCache();
	SCB_InvalidateICache();
	return start - stop;
}

static void print_speed(int32_t stick)
{
	if (stick == 0) {
		printf(" %12c", '-');
		return;
	}
	if (stick < 0)
		stick += data.stick.reload + 1;

	double bytes = MEM_SIZE * MEM_REPEAT;
	double time = (double)stick / (double)data.stick.freq;
	double bps = bytes / time;

	static const char *unit[] = {ESC_READ " %9.3fB/s", ESC_YELLOW " %9.3fK/s",
				     ESC_GREEN " %9.3fM/s", ESC_CYAN " %9.3fG/s"};
	uint32_t uidx = 0;
	while (bps > 1024) {
		bps /= 1024;
		uidx++;
	}
	printf(unit[uidx], bps);
	printf(ESC_NORMAL);
}

static void test()
{
	for (unsigned int icpy = 0; icpy < ASIZE(copy); icpy++) {
		printf("%12s", copy[icpy].name);
		for (unsigned int isrc = 0; isrc < ASIZE(region); isrc++)
			printf(" %12s", region[isrc].name);
		putchar('\n');
		for (unsigned int idst = 0; idst < ASIZE(region); idst++) {
			if (!(region[idst].access & ACCESS_W))
				continue;
			printf("%12s", region[idst].name);
			for (unsigned int isrc = 0; isrc < ASIZE(region); isrc++) {
				flushCache();
				print_speed((*copy[icpy].func)((void *)region[idst].addr,
							       (void *)region[isrc].addr,
							       MEM_SIZE));
			}
			putchar('\n');
		}
		putchar('\n');
	}
}

static void print_info()
{
	printf("Test size:\t%u bytes\n", MEM_SIZE);
	printf("Test repeats:\t%u times\n", MEM_REPEAT);
	printf("ITCM available:\t%lu bytes\n", itcm_size - (&__itcm_end__ - &__itcm_start__));
	printf("DTCM available:\t%lu bytes\n", (uint32_t)stack_ptr - RAMDTCM_BASE);
	putchar('\n');

	for (unsigned int i = 0; i < ASIZE(region); i++)
		printf("Region %u: %12s %c%c %p\n", i, region[i].name,
		       region[i].access & ACCESS_R ? 'r' : '-',
		       region[i].access & ACCESS_W ? 'w' : '-',
		       (void *)region[i].addr);
	putchar('\n');

	for (unsigned int i = 0; i < ASIZE(copy); i++)
		printf("Function %u: %12s\n", i, copy[i].name);
	putchar('\n');

	flushCache();
}

void mem_test()
{
	// Disable all interrupts
	__disable_irq();
	printf(ESC_DEBUG "%lu\tmem_test: Memory test start\n\n" ESC_NORMAL, systick_cnt());
	// SysTick frequency
	data.stick.freq = clkAHB();
	// Set SysTick to maximum reload
	uint32_t stick_ctrl = SysTick->CTRL;
	uint32_t stick_load = SysTick->LOAD;
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
	SysTick->LOAD = -1;
	SysTick->VAL = 0;
	data.stick.reload = SysTick->LOAD;

	// Start test
	flushCache();
	print_info();
	test();

	// Restore systick
	SysTick->CTRL = stick_ctrl;
	SysTick->LOAD = stick_load;
	SysTick->VAL = 0;
	printf(ESC_DEBUG "%lu\tmem_test: Memory test end\n", systick_cnt());
	// Enable interrupts
	__enable_irq();
	flushCache();
}
