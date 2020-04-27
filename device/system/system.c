#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <escape.h>
#include <device.h>
#include <debug.h>
#include <system/systick.h>
#include <system/clocks.h>
#include <system/irq.h>
#include "system.h"

#define STDOUT_BUFFER_SIZE	4096

// Vector table base address
extern const uint32_t g_pfnVectors[];

// Section addresses
extern uint8_t __bss_start__;
extern uint8_t __bss_end__;
extern uint8_t __data_load__;
extern uint8_t __data_start__;
extern uint8_t __data_end__;
extern uint8_t __itcm_load__;
extern uint8_t __itcm_start__;
extern uint8_t __itcm_end__;
extern uint8_t __dmaram_start__;
extern uint8_t __dmaram_end__;

// stdout buffer
static char stdout_buf[STDOUT_BUFFER_SIZE];
static uint32_t i_stdout_read = 0;
static volatile uint32_t i_stdout_write = 0;

// Assembly helper
extern int _main();

// Buffered STDIO
int fio_write(int file, char *ptr, int len)
{
#if DEBUG >= 5
	if (file != STDOUT_FILENO) {
		errno = ENOSYS;
		return -1;
	}
#endif
	// Update buffer pointer
	__disable_irq();
	uint32_t i = i_stdout_write;
	i_stdout_write = (i + len) & (STDOUT_BUFFER_SIZE - 1);
	__enable_irq();

	// Copy data to allocated space
	if (i + len >= STDOUT_BUFFER_SIZE) {
		uint32_t s = STDOUT_BUFFER_SIZE - i;
		memcpy(&stdout_buf[i], ptr, s);
		memcpy(&stdout_buf[0], ptr + s, len - s);
	} else {
		memcpy(&stdout_buf[i], ptr, len);
	}
	return len;
}

int fio_read(int file, char *ptr, int len)
{
	errno = ENOSYS;
	return -1;
}

static inline void debug_uart_init()
{
	// Enable USART6 clock
	RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
	// Configure GPIOs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	// 10: Alternative function mode
	GPIO_MODER(GPIOC, 6, GPIO_MODER_ALTERNATE);
	GPIO_MODER(GPIOC, 7, GPIO_MODER_ALTERNATE);
	GPIO_OTYPER_PP(GPIOC, 6);
	GPIO_OSPEEDR(GPIOC, 6, GPIO_OSPEEDR_LOW);
	GPIO_PUPDR(GPIOC, 7, GPIO_PUPDR_UP);
	// AF8: USART6
	GPIO_AFRL(GPIOC, 6, 8);
	GPIO_AFRL(GPIOC, 7, 8);

	// USART6 uses APB2 clock
	uint32_t clk = clkAPB2();
	// Debug UART baud rate 115200
	uint32_t baud = 115200;
	// Disable UART, 8-bit, N, 1 stop
	USART6->CR1 = 0;
	USART6->CR2 = 0;
	USART6->CR3 = 0;
	USART6->BRR = ((clk << 1) / baud + 1) >> 1;
	// Enable transmitter & receiver & UART
	USART6->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static inline void debug_uart_putchar(int ch)
{
	while (!(USART6->ISR & USART_ISR_TXE));
	USART6->TDR = ch;
}

static inline int debug_uart_getchar(void)
{
	while (!(USART6->ISR & USART_ISR_RXNE));
	return USART6->RDR;
}

static void dma_memcpy(void *dst, void *src, uint32_t len)
{
	if (len == 0)
		return;
#if DEBUG >= 5
	if (len & 3)
		dbgbkpt();
#endif

	// Number of data items, assuming 32-bit transfers
	DMA2_Stream0->NDTR = len >> 2;
	// Peripheral address, i.e. source address
	DMA2_Stream0->PAR = (uint32_t)src;
	// Memory address, i.e. destination address
	DMA2_Stream0->M0AR = (uint32_t)dst;
	// DMA stream enable
	DMA2_Stream0->CR |= DMA_SxCR_EN_Msk;

	// Wait for transfer complete
	while (DMA2_Stream0->CR & DMA_SxCR_EN_Msk)
		__WFE();
#if DEBUG >= 5
	if ((DMA2->LISR & DMA_LISR_TCIF0_Msk) == 0)
		dbgbkpt();
#endif
	// Clear interrupt pending
	DMA2->LIFCR = DMA_LIFCR_CTCIF0_Msk;
	NVIC_ClearPendingIRQ(DMA2_Stream0_IRQn);
}

// Reset entry point, initialise the system
void Reset_Handler(void)
{
	// Initialise stask pointer
	asm("ldr sp, =__stack_end__");

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
	// FPU settings
	// Set CP10 and CP11 Full Access
	SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));
#endif
	// Reset the RCC clock configuration to the default reset state
	// Set HSION bit
	RCC->CR |= RCC_CR_HSION_Msk;
	// Reset CFGR register
	RCC->CFGR = 0x00000000;
	// Reset HSEON, CSSON and PLLON bits
	RCC->CR &= (uint32_t)0xFEF6FFFF;
	// Reset PLLCFGR register
	RCC->PLLCFGR = 0x24003010;
	// Reset HSEBYP bit
	RCC->CR &= (uint32_t)0xFFFBFFFF;
	// Disable all interrupts
	RCC->CIR = 0x00000000;
	// Configure the Vector Table location add offset address
	SCB->VTOR = (uint32_t)g_pfnVectors;

	// System initialisation
	rcc_init();
	NVIC_SetPriorityGrouping(NVIC_PRIORITY_GROUPING);
	systick_init(1000);
	debug_uart_init();

	// Initialise DMA for memory copy
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN_Msk;
	// Memory-to-memory, single 32-bit trasnfer, auto increment, transfer complete interrupt
	DMA2_Stream0->CR = (0b10 << DMA_SxCR_DIR_Pos) | (0b10 << DMA_SxCR_MSIZE_Pos) |
			(0b10 << DMA_SxCR_PSIZE_Pos) | DMA_SxCR_TCIE_Msk |
			DMA_SxCR_MINC_Msk | DMA_SxCR_PINC_Msk;
	// DMA FIFO half full threshold
	DMA2_Stream0->FCR = 0b01 << DMA_SxFCR_FTH_Pos;
	// Send event on interrupt pending
	SCB->SCR |= SCB_SCR_SEVONPEND_Msk;

	// Initialise data from flash to SRAM using DMA
	memcpy(&__data_start__, &__data_load__, &__data_end__ - &__data_start__);
	memcpy(&__itcm_start__, &__itcm_load__, &__itcm_end__ - &__itcm_start__);

	// Deinitialise DMA
	RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA2EN_Msk;

	// Zero fill bss segment
	// SRAM can be accessed as full words (32 bits)
	uint32_t *p = (uint32_t *)&__bss_start__;
	while (p != (uint32_t *)&__bss_end__)
		*p++ = 0;

	// MPU region definition
	const ARM_MPU_Region_t mpu_regions[] = {
		// Disable write cache on DMA RAM
		{.RBAR = ARM_MPU_RBAR(0, (uint32_t)&__dmaram_start__),
		 .RASR = ARM_MPU_RASR(1, ARM_MPU_AP_PRIV, 0b000, 1, 0, 1, 0x00, ARM_MPU_REGION_SIZE_16KB)},
	};

	// Initialise MPU and cache
	ARM_MPU_Load(mpu_regions, sizeof(mpu_regions) / sizeof(ARM_MPU_Region_t));
	ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);		// Fallback to default mapping
	SCB_EnableICache();
	SCB_EnableDCache();

	// Start program
	(&_main)();
}

// Default handler for unexpected interrupts
void debug_handler()
{
#if DEBUG >= 5
	SCB_Type scb = *SCB;
	NVIC_Type nvic = *NVIC;
#endif
#if DEBUG
	uint32_t ipsr = __get_IPSR();
	printf(ESC_ERROR "\nUnexpected interrupt: %lu\n", ipsr);
	flushCache();
#endif
#if DEBUG >= 5
	dbgbkpt();
#endif
	NVIC_SystemReset();
}

#if DEBUG
#ifndef BOOTLOADER
void HardFault_Handler()
{
	SCB_Type scb = *SCB;
	printf(ESC_ERROR "\nHard fault: 0x%08lx\n", scb.HFSR);
	printf("...\t%s%s%s\n",
	       scb.HFSR & SCB_HFSR_DEBUGEVT_Msk ? "Debug " : "",
	       scb.HFSR & SCB_HFSR_FORCED_Msk ? "Forced " : "",
	       scb.HFSR & SCB_HFSR_VECTTBL_Msk ? "Vector " : "");
	uint8_t mfsr = FIELD(scb.CFSR, SCB_CFSR_MEMFAULTSR);
	if (mfsr & 0x80) {	// Memory manage fault valid
		printf("Memory manage fault: 0x%02x\n", mfsr);
		printf("...\t%s%s%s%s\n",
		       mfsr & 0x10 ? "Entry " : "",
		       mfsr & 0x08 ? "Return " : "",
		       mfsr & 0x02 ? "Data " : "",
		       mfsr & 0x01 ? "Instruction " : "");
		printf("...\tAddress: 0x%08lx\n", scb.MMFAR);
	}
	uint8_t bfsr = FIELD(scb.CFSR, SCB_CFSR_BUSFAULTSR);
	if (bfsr & 0x80) {	// Bus fault valid
		printf("Bus fault: 0x%02x\n", bfsr);
		printf("...\t%s%s%s%s%s\n",
		       bfsr & 0x10 ? "Entry " : "",
		       bfsr & 0x08 ? "Return " : "",
		       bfsr & 0x04 ? "Imprecise " : "",
		       bfsr & 0x02 ? "Precise " : "",
		       bfsr & 0x01 ? "Instruction " : "");
		if (bfsr & 0x02)
			printf("...\tPrecise: 0x%08lx\n", scb.BFAR);
	}
	uint16_t ufsr = FIELD(scb.CFSR, SCB_CFSR_USGFAULTSR);
	if (ufsr) {	// Usage fault
		printf("Usage fault: 0x%04x\n", ufsr);
		printf("...\t%s%s%s%s%s%s\n",
		       ufsr & 0x0200 ? "Divide " : "",
		       ufsr & 0x0100 ? "Unaligned " : "",
		       ufsr & 0x0008 ? "Coprocessor " : "",
		       ufsr & 0x0004 ? "INVPC " : "",
		       ufsr & 0x0002 ? "INVSTATE " : "",
		       ufsr & 0x0001 ? "UNDEFINSTR " : "");
	}

	dbgbkpt();
	NVIC_SystemReset();
}
#endif
#endif

void system_debug_process()
{
	// Flush stdout buffer to debug uart
	while (i_stdout_read != i_stdout_write) {
		char c = stdout_buf[i_stdout_read];
		if (c == '\n')
			debug_uart_putchar('\r');
		debug_uart_putchar(c);
		i_stdout_read = (i_stdout_read + 1) & (STDOUT_BUFFER_SIZE - 1);
	}
}

IDLE_HANDLER(&system_debug_process);

void flushCache()
{
	fflush(stdout);		// Note, this is not possible from ITCM
	system_debug_process();
	SCB_CleanInvalidateDCache();
}
