#include <string.h>
#include <stm32f7xx.h>
#include "mmc.h"
#include "mmc_defs.h"
#include "../systick.h"
#include "../clocks.h"
#include "../macros.h"
#include "../debug.h"

#define MMC	SDMMC1
#define STREAM	DMA2_Stream3

typedef union {
	struct PACKED {
		uint8_t mid;
		uint16_t oid;
		char pnm[5];
		uint8_t prv;
		uint32_t psn;
		uint16_t mdt;
		uint8_t crc;
	};
	uint32_t raw[4];
} cid_t;

static DSTATUS stat = STA_NOINIT;
static uint32_t ccs = 0, capacity = 0, blocks = 0, rca = 0;
enum {MMCIdle = 0, MMCRead, MMCWrite, MMCMulti = 0x80} status;

// Send command to card, with optional status output
// Returns short response
static uint32_t mmc_command(uint32_t cmd, uint32_t arg, uint32_t *stat);
// RCA: [31:16] RCA, [15:0] stuff bits
static uint32_t mmc_app_command(uint32_t cmd, uint32_t rca, uint32_t arg, uint32_t *stat);

static inline void mmc_base_init()
{
	// Enable IO compensation cell
	if (!(SYSCFG->CMPCR & SYSCFG_CMPCR_READY)) {
		RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
		SYSCFG->CMPCR |= SYSCFG_CMPCR_CMP_PD;
	}
	// Initialise GPIOs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN_Msk |
			RCC_AHB1ENR_GPIOCEN_Msk | RCC_AHB1ENR_GPIODEN_Msk;
#if HWVER == 0x0002
	// PA12, CD, input
	GPIO_MODER(GPIOA, 12, 0b00);
	GPIO_PUPDR(GPIOA, 12, GPIO_PUPDR_UP);
#else
	// PA15, CD, input
	GPIO_MODER(GPIOA, 15, 0b00);
	GPIO_PUPDR(GPIOA, 15, GPIO_PUPDR_UP);
#endif
	// PC8, D0, alternative function
	GPIO_MODER(GPIOC, 8, 0b10);
	GPIO_PUPDR(GPIOC, 8, GPIO_PUPDR_UP);
	GPIO_OTYPER_PP(GPIOC, 8);	// Output push-pull
	GPIO_OSPEEDR(GPIOC, 8, 0b10);	// High speed (50~100MHz)
	GPIO_AFRH(GPIOC, 8, 12);	// AF12: SDMMC1
	// PC9, D1, alternative function
	GPIO_MODER(GPIOC, 9, 0b10);
	GPIO_PUPDR(GPIOC, 9, GPIO_PUPDR_UP);
	GPIO_OTYPER_PP(GPIOC, 9);	// Output push-pull
	GPIO_OSPEEDR(GPIOC, 9, 0b10);	// High speed (50~100MHz)
	GPIO_AFRH(GPIOC, 9, 12);	// AF12: SDMMC1
	// PC10, D2, alternative function
	GPIO_MODER(GPIOC, 10, 0b10);
	GPIO_PUPDR(GPIOC, 10, GPIO_PUPDR_UP);
	GPIO_OTYPER_PP(GPIOC, 10);	// Output push-pull
	GPIO_OSPEEDR(GPIOC, 10, 0b10);	// High speed (50~100MHz)
	GPIO_AFRH(GPIOC, 10, 12);	// AF12: SDMMC1
	// PC11, D3, alternative function
	GPIO_MODER(GPIOC, 11, 0b10);
	GPIO_PUPDR(GPIOC, 11, GPIO_PUPDR_UP);
	GPIO_OTYPER_PP(GPIOC, 11);	// Output push-pull
	GPIO_OSPEEDR(GPIOC, 11, 0b10);	// High speed (50~100MHz)
	GPIO_AFRH(GPIOC, 11, 12);	// AF12: SDMMC1
	// PC12, CK, alternative function
	GPIO_MODER(GPIOC, 12, 0b10);
	GPIO_PUPDR(GPIOC, 12, GPIO_PUPDR_UP);
	GPIO_OTYPER_PP(GPIOC, 12);	// Output push-pull
	GPIO_OSPEEDR(GPIOC, 12, 0b10);	// High speed (50~100MHz)
	GPIO_AFRH(GPIOC, 12, 12);	// AF12: SDMMC1
	// PD2, CMD, alternative function
	GPIO_MODER(GPIOD, 2, 0b10);
	GPIO_PUPDR(GPIOD, 2, GPIO_PUPDR_UP);
	GPIO_OTYPER_PP(GPIOD, 2);	// Output push-pull
	GPIO_OSPEEDR(GPIOD, 2, 0b10);	// High speed (50~100MHz)
	GPIO_AFRL(GPIOD, 2, 12);	// AF12: SDMMC1
	// Wait for IO compensation cell
	while (!(SYSCFG->CMPCR & SYSCFG_CMPCR_READY));

	// DMA initialisation
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN_Msk;
	// Disable stream
	STREAM->CR = 0ul;
	// Peripheral to memory, 32bit -> 32bit, burst of 4 beats, low priority
	STREAM->CR = (4ul << DMA_SxCR_CHSEL_Pos) | (0b00ul << DMA_SxCR_PL_Pos) |
			(0b01ul << DMA_SxCR_MBURST_Pos) | (0b01ul << DMA_SxCR_PBURST_Pos) |
			(0b10ul << DMA_SxCR_MSIZE_Pos) | (0b10ul << DMA_SxCR_PSIZE_Pos) |
			(0b00ul << DMA_SxCR_DIR_Pos) | DMA_SxCR_MINC_Msk | DMA_SxCR_PFCTRL_Msk;
	// Peripheral address
	STREAM->PAR = (uint32_t)&MMC->FIFO;
	// FIFO control
	STREAM->FCR = DMA_SxFCR_DMDIS_Msk | (0b11 << DMA_SxFCR_FTH_Pos);
}

static uint32_t mmc_command(uint32_t cmd, uint32_t arg, uint32_t *stat)
{
	MMC->ARG = arg;
	MMC->CMD = cmd | SDMMC_CMD_CPSMEN_Msk;

	if (!(cmd & SDMMC_CMD_WAITRESP_0))
		// No response expected
		while (!(MMC->STA & SDMMC_STA_CMDSENT_Msk));
	else
		while (!(MMC->STA & (SDMMC_STA_CMDREND_Msk |
				     SDMMC_STA_CCRCFAIL_Msk |
				     SDMMC_STA_CTIMEOUT_Msk)));
	if (stat)
		*stat = MMC->STA;

	// Clear flags
	MMC->ICR = SDMMC_ICR_CMDSENTC_Msk | SDMMC_ICR_CMDRENDC_Msk |
			SDMMC_ICR_CCRCFAILC_Msk | SDMMC_ICR_CTIMEOUTC_Msk;
	if (!(cmd & SDMMC_CMD_WAITRESP_0))
		return MMC->RESPCMD;
	else
		return MMC->RESP1;
}

static uint32_t mmc_app_command(uint32_t cmd, uint32_t rca, uint32_t arg, uint32_t *stat)
{
	uint32_t s;
	uint32_t resp = mmc_command(APP_CMD, rca, &s);
	if (!(s & SDMMC_STA_CMDREND_Msk) || (resp & STAT_ERROR)) {
		if (stat)
			*stat = s;
		dbgbkpt();
		return 0;
	}
	return mmc_command(cmd, arg, stat);
}

uint32_t mmc_capacity()
{
	return capacity;
}

uint32_t mmc_statistics()
{
	return blocks;
}

static uint32_t mmc_read_block(void *p, uint32_t start, uint32_t count)
{
	if (count == 0)
		return 0u;

	// Polling for card busy
	uint32_t resp, stat;
	do {
		resp = mmc_command(SEND_STATUS, rca, &stat);
		if (!(stat & SDMMC_STA_CMDREND_Msk)) {
			dbgbkpt();
			return 0;
		}
	} while (resp >> 9u != 4u);

	// Clear SDMMC flags
	MMC->ICR = SDMMC_ICR_DBCKENDC_Msk | SDMMC_ICR_DATAENDC_Msk |
			SDMMC_ICR_RXOVERRC_Msk;

	// Clear DMA interrupts
	DMA2->LIFCR = DMA_LIFCR_CTCIF3_Msk | DMA_LIFCR_CHTIF3_Msk |
			DMA_LIFCR_CTEIF3_Msk | DMA_LIFCR_CDMEIF3_Msk;
	// Set memory address
	STREAM->M0AR = (uint32_t)p;
	// Peripheral to memory, 32bit -> 32bit, burst of 4 beats, low priority
	STREAM->CR = (4ul << DMA_SxCR_CHSEL_Pos) | (0b00ul << DMA_SxCR_PL_Pos) |
			(0b01ul << DMA_SxCR_MBURST_Pos) | (0b01ul << DMA_SxCR_PBURST_Pos) |
			(0b10ul << DMA_SxCR_MSIZE_Pos) | (0b10ul << DMA_SxCR_PSIZE_Pos) |
			(0b00ul << DMA_SxCR_DIR_Pos) | DMA_SxCR_MINC_Msk | DMA_SxCR_PFCTRL_Msk;
	// Enable DMA stream
	STREAM->CR |= DMA_SxCR_EN_Msk;

	// Set transfer length
	MMC->DLEN = count * 512ul;
	// Enable data transfer
	MMC->DCTRL = SDMMC_DCTRL_DTDIR_Msk | SDMMC_DCTRL_DTEN_Msk |
			SDMMC_DCTRL_DMAEN_Msk |
			(9ul << SDMMC_DCTRL_DBLOCKSIZE_Pos);

	// Correct start address for SDSC cards
	if (ccs == 0)
		start *= 512u;

	// Send command
	resp = mmc_command(count == 1 ? READ_SINGLE_BLOCK : READ_MULTIPLE_BLOCK, start, &stat);
	if (!(stat & SDMMC_STA_CMDREND_Msk)) {
		dbgbkpt();
		return 0;
	}

	// Wait for data block reception
	while (!(MMC->STA & SDMMC_STA_DATAEND_Msk));

	// Stop transmission for multiple block operation
	if (count != 1) {
		resp = mmc_command(STOP_TRANSMISSION, 0, &stat);
		if (!(stat & SDMMC_STA_CMDREND_Msk)) {
			dbgbkpt();
			return 0;
		}
	}

	// Statistics
	blocks += count;
	return count;
}

static uint32_t mmc_write_start(uint32_t start, uint32_t count)
{
	if (count == 0)
		return 0u;

	// Polling for card busy
	uint32_t stat, resp;
	do {
		resp = mmc_command(SEND_STATUS, rca, &stat);
		if (!(stat & SDMMC_STA_CMDREND_Msk)) {
			dbgbkpt();
			return 0;
		}
	} while (resp >> 9u != 4u);

	// Clear SDMMC flags
	MMC->ICR = SDMMC_ICR_DBCKENDC_Msk | SDMMC_ICR_DATAENDC_Msk |
			SDMMC_ICR_TXUNDERRC_Msk | SDMMC_ICR_RXOVERRC_Msk;
	// Stop DPSM
	MMC->DCTRL = 0;
	// Clear DMA interrupts
	DMA2->LIFCR = DMA_LIFCR_CTCIF3_Msk | DMA_LIFCR_CHTIF3_Msk |
			DMA_LIFCR_CTEIF3_Msk | DMA_LIFCR_CDMEIF3_Msk;
	// Memory to peripheral, 32bit -> 32bit, burst of 4 beats, low priority
	STREAM->CR = (4ul << DMA_SxCR_CHSEL_Pos) | (0b00ul << DMA_SxCR_PL_Pos) |
			(0b01ul << DMA_SxCR_MBURST_Pos) | (0b01ul << DMA_SxCR_PBURST_Pos) |
			(0b10ul << DMA_SxCR_MSIZE_Pos) | (0b10ul << DMA_SxCR_PSIZE_Pos) |
			(0b01ul << DMA_SxCR_DIR_Pos) | DMA_SxCR_MINC_Msk | DMA_SxCR_PFCTRL_Msk;

	// Correct start address for SDSC cards
	if (ccs == 0)
		start *= 512u;

	// Pre-erase blocks
	if (count != 1) {
		resp = mmc_app_command(SET_WR_BLK_ERASE_COUNT, rca, count, &stat);
		if (!(stat & (SDMMC_STA_CMDREND_Msk))) {
			dbgbkpt();
			return 0;
		}
	}

	// Start writing
	resp = mmc_command(count == 1 ? WRITE_BLOCK : WRITE_MULTIPLE_BLOCK, start, &stat);
	if (!(stat & (SDMMC_STA_CMDREND_Msk))) {
		dbgbkpt();
		return 0;
	}

	// Update status
	dbgprintf(ESC_RED "[MMC] Start writing: %lu + %lu\n", start, count);
	status = (count == 1 ? 0 : MMCMulti) | MMCWrite;
	return count;
}

static uint32_t mmc_write_data(const void *p, uint32_t count)
{
	SDMMC_TypeDef *mmc = MMC;
	DMA_TypeDef *dma = DMA2;
	DMA_Stream_TypeDef *stream = STREAM;
	SCB_CleanInvalidateDCache();
	dbgprintf(ESC_RED "[MMC] Writing data: %p => %lu\n", p, count);

	// Set transfer length
	MMC->DLEN = count * 512ul;
	// Setup data transfer
	MMC->DCTRL = SDMMC_DCTRL_DTEN_Msk | /*SDMMC_DCTRL_DMAEN_Msk |*/
			(9ul << SDMMC_DCTRL_DBLOCKSIZE_Pos);

	// Data transfer
	for (uint32_t i = count * (512ul / 4ul); i; i--, p += 4u) {
		while (!(MMC->STA & SDMMC_STA_TXFIFOHE_Msk));
		MMC->FIFO = *(uint32_t *)p;
	}
	// Wait for data block transmission
	while (!(MMC->STA & SDMMC_STA_DATAEND_Msk));
	while (MMC->STA & SDMMC_STA_TXACT_Msk);

	// Statistics
	blocks += count;
	return count;
}

static uint32_t mmc_write_stop()
{
	// Stop transmission for multiple block operation
	if (status & MMCMulti) {
		// Wait for data block transmission
		while (!(MMC->STA & SDMMC_STA_DATAEND_Msk));

		uint32_t resp, stat;
		resp = mmc_command(STOP_TRANSMISSION, 0, &stat);
		if (!(stat & SDMMC_STA_CMDREND_Msk)) {
			dbgbkpt();
			status = MMCIdle;
			return 0;
		}
	}

	// Clear SDMMC flags
	MMC->ICR = SDMMC_ICR_DBCKENDC_Msk | SDMMC_ICR_DATAENDC_Msk |
			SDMMC_ICR_TXUNDERRC_Msk | SDMMC_ICR_RXOVERRC_Msk;

	dbgprintf(ESC_RED "[MMC] Data transfer finished\n");
	status = MMCIdle;
	return 1;
}

static uint32_t mmc_write_block(const void *p, uint32_t start, uint32_t count)
{
	mmc_write_start(start, count);
	mmc_write_data(p, count);
	mmc_write_stop();
	return count;
}

/* FatFs interface functions */

DSTATUS mmc_disk_init()
{
	if (stat == 0)
		return stat;

	if (stat & STA_NOINIT) {
		mmc_base_init();
		// Initialise SDMMC module
		RCC->APB2ENR |= RCC_APB2ENR_SDMMC1EN_Msk;
	}

	// Switch opens when card inserted
#if HWVER == 0x0002
	if (GPIOA->IDR & (1ul << 12))
		stat = STA_ERROR;
#else
	if (GPIOA->IDR & (1ul << 15))
		stat = STA_ERROR;
#endif
	else {
		// Stop card clock
		MMC->POWER = 0b00ul << SDMMC_POWER_PWRCTRL_Pos;
		stat = STA_NODISK;
		return stat;
	}

	// Card entering identification mode
	// Disable interrupts
	MMC->MASK = 0ul;
	// 1-bit bus, clock enable, 400kHz
	MMC->CLKCR = SDMMC_CLKCR_CLKEN_Msk | (CEIL(clkSDMMC1(), 400000ul) - 2ul);
	// Start card clock
	MMC->POWER = 0b11ul << SDMMC_POWER_PWRCTRL_Pos;
	// The last busy in any write operation up to 500ms
	MMC->DTIMER = clkSDMMC1() / 2ul;
	systick_delay(2);

	// Reset card
	mmc_command(GO_IDLE_STATE, 0, 0);

	// Verify operation (2.7-3.6V, check pattern 0xa5)
	uint32_t sta, arg = (0b0001ul << 8u) | (0xa5 << 0u);
	uint32_t resp = mmc_command(SEND_IF_COND, arg, &sta);
	if (!(sta & SDMMC_STA_CMDREND_Msk)) {
		// Voltage mismatch or Ver.1 or not SD
		dbgbkpt();
		return sta;
	}

	// SD version 2.0 or later
	if (resp != arg) {
		// Unusable card
		dbgbkpt();
		return sta;
	}

	// Inquiry OCR
	uint32_t ocr = mmc_app_command(SD_SEND_OP_COND, 0, 0, 0);
	if (~ocr & (0b11 << 20u)) {
		// 3.2~3.4V unsupported
		dbgbkpt();
		return sta;
	}

	// OCR initialisation, HCS, XPC
	arg = OCR_HCS | OCR_XPC | (0b11 << 20u);
	// Wait for busy status
	while (!((ocr = mmc_app_command(SD_SEND_OP_COND, 0, arg, 0)) & OCR_BUSY));
	// Version 2.00, standard or extended capacity card
	ccs = ocr & OCR_CCS;

	// No need for 1.8V voltage switching

	// Read CID register
	mmc_command(ALL_SEND_CID, 0, &sta);
	// Publish relative address
	rca = mmc_command(SEND_RELATIVE_ADDR, 0, &sta);
	if (!(sta & SDMMC_STA_CMDREND_Msk) || (rca & STAT_ERROR)) {
		dbgbkpt();
		return sta;
	}

	// Read CID register
	mmc_command(ALL_SEND_CID, 0, &sta);
	if (!(sta & SDMMC_STA_CTIMEOUT_Msk)) {
		// More than 1 card?
		dbgbkpt();
	}

	// Initialised, switch to high-speed clock
	uint32_t clk;
	if (!ccs)	// SDSC, up to 25MHz
		clk = CEIL(clkSDMMC1(), 25000000ul) - 2ul;
	else		// SDHC/SDXC, up to 50MHz
		clk = SDMMC_CLKCR_BYPASS_Msk;
	// Flow control, 4-bit bus, power saving, clock enable
	MMC->CLKCR = SDMMC_CLKCR_HWFC_EN_Msk | (0b01ul << SDMMC_CLKCR_WIDBUS_Pos) |
			SDMMC_CLKCR_NEGEDGE_Msk |
			SDMMC_CLKCR_PWRSAV_Msk | SDMMC_CLKCR_CLKEN_Msk | clk;

	// Read CSD register
	mmc_command(SEND_CSD, rca, &sta);
	if (!(sta & SDMMC_STA_CMDREND_Msk)) {
		dbgbkpt();
		return sta;
	}

	if ((MMC->RESP1 >> 30u) == 0u) {	// CSD V1
		// Calculate card capacity from CSD register
		// [73:62] C_SIZE[12]
		uint32_t size = (((MMC->RESP2 << 2u) | (MMC->RESP3 >> 30u))
				 & 0x0fffu) + 1u;
		// [49:47] C_SIZE_MULT[3]
		uint32_t mult = 1ul << (((MMC->RESP3 >> 15u) & 0x07u) + 2u);
		// [83:80] READ_BL_LEN[4]
		uint32_t len = 1ul << ((MMC->RESP2 >> 16u) & 0x0fu);
		capacity = size * mult * len / 512ul;
	} else {				// CSD V2
		// Calculate card capacity from CSD register
		// [69:48] C_SIZE[22]
		capacity = ((((MMC->RESP2 << 16u) | (MMC->RESP3 >> 16u))
			     & 0x003ffffful) + 1ul) * 1024ul;
	}

	// Select card
	resp = mmc_command(SELECT_CARD, rca, &sta);
	if (!(sta & SDMMC_STA_CMDREND_Msk) || (resp & STAT_ERROR)) {
		dbgbkpt();
		return sta;
	}

	// Set bus width (4 bits bus)
	resp = mmc_app_command(SET_BUS_WIDTH, rca, 0b10, &sta);
	if (!(sta & SDMMC_STA_CMDREND_Msk) || (resp & STAT_ERROR)) {
		dbgbkpt();
		return sta;
	}

	// Unlock card
	while (resp & STAT_CARD_IS_LOCKED) {
		resp = mmc_command(LOCK_UNLOCK, 0, &sta);
		dbgbkpt();
	}

	// Set block length
	resp = mmc_command(SET_BLOCKLEN, 512u, &sta);
	if (!(sta & SDMMC_STA_CMDREND_Msk) || (resp & STAT_ERROR)) {
		dbgbkpt();
		return sta;
	}

	stat = 0;
	return stat;
}

DSTATUS mmc_disk_status()
{
	return stat;
}

DRESULT mmc_disk_read(BYTE *buff, DWORD sector, UINT count)
{
	if (stat)
		return RES_ERROR;
	if (mmc_read_block(buff, sector, count) != count)
		return RES_ERROR;
	return RES_OK;
}

/* SCSI interface functions */

uint8_t scsi_buf[64 * 1024] ALIGN(32);

uint32_t scsi_capacity(scsi_t *scsi, uint32_t *lbnum, uint32_t *lbsize)
{
	*lbnum = capacity;
	*lbsize = 512ul;
	return 0;
}

void *scsi_read(scsi_t *scsi, uint32_t offset, uint32_t *length)
{
	// TODO: Partial read
	if (*length > sizeof(scsi_buf))
		panic();

	// Invalidate data cache for DMA operation
	SCB_InvalidateDCache_by_Addr((void *)scsi_buf, *length);

	*length = mmc_read_block(scsi_buf, offset / 512ul, *length / 512ul) * 512ul;
	return scsi_buf;
}

uint32_t scsi_write_start(scsi_t *scsi, uint32_t offset, uint32_t size)
{
	// Check block alignment
	if (size & (512ul - 1ul)) {
		dbgbkpt();
		return 0;
	} else if (offset & (512ul - 1ul)) {
		dbgbkpt();
		return 0;
	}
	return mmc_write_start(offset / 512ul, size / 512ul) * 512ul;
}

uint32_t scsi_write_data(scsi_t *scsi, uint32_t length, const void *p)
{
	// Check block alignment
	if (length & (512ul - 1ul)) {
		dbgbkpt();
		return 0;
	}
	return mmc_write_data(p, length / 512ul) * 512ul;
}

uint32_t scsi_write_stop(scsi_t *scsi)
{
	return mmc_write_stop();
}
