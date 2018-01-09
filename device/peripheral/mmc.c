#include <string.h>
#include <stm32f7xx.h>
#include <systick.h>
#include <clocks.h>
#include <macros.h>
#include <escape.h>
#include <debug.h>
#include <usb/msc/scsi_defs_sense.h>
#include "mmc.h"
#include "mmc_defs.h"

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
enum {MMCIdle = 0, MMCRead, MMCWrite, MMCMulti = 0x80} status = MMCIdle;

uint32_t mmc_capacity()
{
	return capacity;
}

uint32_t mmc_statistics()
{
	return blocks;
}

void mmc_reset(DSTATUS s)
{
	if (status)
		capacity = 0;
	status = MMCIdle;
	stat = s;
}

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

static DSTATUS mmc_cd()
{
	// Switch opens when card inserted
#if HWVER == 0x0002
	uint32_t cd_mask = 1ul << 12;
#else
	uint32_t cd_mask = 1ul << 15;
#endif
	// Debouncing
	if ((GPIOA->IDR & cd_mask) && (stat & STA_NODISK))
		systick_delay(10);

	if (GPIOA->IDR & cd_mask) {
		if (stat & STA_NODISK)
			mmc_reset(stat & ~STA_NODISK);
		return stat;
	}

	// Disable SDMMC module
	RCC->APB2ENR &= ~RCC_APB2ENR_SDMMC1EN_Msk;

	mmc_reset(STA_NODISK);
	blocks = 0;
	return stat;
}

static uint32_t mmc_command(uint32_t cmd, uint32_t arg, uint32_t *stat)
{
	MMC->ARG = arg;
	MMC->CMD = cmd | SDMMC_CMD_CPSMEN_Msk;

	// Response expected
	uint32_t flag;
	if (cmd & SDMMC_CMD_WAITRESP_0)
		flag = SDMMC_STA_CMDREND_Msk;
	else
		flag = SDMMC_STA_CMDSENT_Msk;

	uint32_t tick = systick_cnt();
	while (!(MMC->STA & (flag | SDMMC_STA_CCRCFAIL_Msk |
			     SDMMC_STA_CTIMEOUT_Msk)) &&
			systick_cnt() - tick < 100ul);

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

static uint32_t mmc_ping()
{
	// Not in transfer mode
	if (status != MMCIdle) {
		uint32_t sta = MMC->STA;
		if (sta & (SDMMC_STA_DCRCFAIL_Msk | SDMMC_STA_DTIMEOUT_Msk |
			   SDMMC_STA_CCRCFAIL_Msk | SDMMC_STA_CTIMEOUT_Msk))
			return STA_ERROR;
		return 0;
	}

	uint32_t stat, resp;
	resp = mmc_command(SEND_STATUS, rca, &stat);
	if (!(stat & SDMMC_STA_CMDREND_Msk)) {
		dbgprintf(ESC_ERROR "[MMC] Response timed out\n");
		return STA_ERROR;
	}
	return 0;
}

static uint32_t mmc_read_prepare()
{
	if (status != MMCIdle) {
		dbgbkpt();
		return 1;
	}

	// Peripheral to memory, 32bit -> 32bit, burst of 4 beats, low priority
	STREAM->CR = (4ul << DMA_SxCR_CHSEL_Pos) | (0b00ul << DMA_SxCR_PL_Pos) |
			(0b01ul << DMA_SxCR_MBURST_Pos) | (0b01ul << DMA_SxCR_PBURST_Pos) |
			(0b10ul << DMA_SxCR_MSIZE_Pos) | (0b10ul << DMA_SxCR_PSIZE_Pos) |
			(0b00ul << DMA_SxCR_DIR_Pos) | DMA_SxCR_MINC_Msk | DMA_SxCR_PFCTRL_Msk;

	// Polling for card busy
	uint32_t resp, stat;
	do {
		resp = mmc_command(SEND_STATUS, rca, &stat);
		if (!(stat & SDMMC_STA_CMDREND_Msk))
			return 2;
	} while (resp >> 9u != 4u);

	// Update status
	status = MMCRead;
	return 0;
}

static uint32_t mmc_read_start(uint32_t start, uint32_t count)
{
	if (status != MMCRead) {
		dbgbkpt();
		return 0;
	}

	if (count == 0) {
		status = MMCIdle;
		return 0;
	}

	// Correct start address for SDSC cards
	if (ccs == 0)
		start *= 512u;

	// Send command to start transfer
	uint32_t resp, stat;
	resp = mmc_command(count == 1 ? READ_SINGLE_BLOCK : READ_MULTIPLE_BLOCK, start, &stat);
	if (!(stat & SDMMC_STA_CMDREND_Msk)) {
		status = MMCIdle;
		return 0;
	}

	// Update status
	status = (count == 1 ? 0 : MMCMulti) | MMCRead;
	return count;
}

static uint32_t mmc_write_start(uint32_t start, uint32_t count)
{
	if (status != MMCIdle) {
		dbgbkpt();
		return 0;
	}

	if (count == 0) {
		status = MMCIdle;
		return 0;
	}

	// Memory to peripheral, 32bit -> 32bit, burst of 4 beats, low priority
	STREAM->CR = (4ul << DMA_SxCR_CHSEL_Pos) | (0b00ul << DMA_SxCR_PL_Pos) |
			(0b01ul << DMA_SxCR_MBURST_Pos) | (0b01ul << DMA_SxCR_PBURST_Pos) |
			(0b10ul << DMA_SxCR_MSIZE_Pos) | (0b10ul << DMA_SxCR_PSIZE_Pos) |
			(0b01ul << DMA_SxCR_DIR_Pos) | DMA_SxCR_MINC_Msk | DMA_SxCR_PFCTRL_Msk;

	// Correct start address for SDSC cards
	if (ccs == 0)
		start *= 512u;

	// Polling for card busy
	uint32_t stat, resp;
	do {
		resp = mmc_command(SEND_STATUS, rca, &stat);
		if (!(stat & SDMMC_STA_CMDREND_Msk)) {
			status = MMCIdle;
			return 0;
		}
	} while (resp >> 9u != 4u);

	// Pre-erase blocks
	if (count != 1) {
		resp = mmc_app_command(SET_WR_BLK_ERASE_COUNT, rca, count, &stat);
		if (!(stat & (SDMMC_STA_CMDREND_Msk))) {
			status = MMCIdle;
			return 0;
		}
	}

	// Start writing
	resp = mmc_command(count == 1 ? WRITE_BLOCK : WRITE_MULTIPLE_BLOCK, start, &stat);
	if (!(stat & (SDMMC_STA_CMDREND_Msk))) {
		status = MMCIdle;
		return 0;
	}

	// Update status
	status = (count == 1 ? 0 : MMCMulti) | MMCWrite;
	return count;
}

static uint32_t mmc_data(const void *p, uint32_t count)
{
	if (!(status & (MMCWrite | MMCRead))) {
		dbgbkpt();
		return 0;
	}

	// Wait for data block transmission
	while (MMC->STA & (SDMMC_STA_TXACT_Msk | SDMMC_STA_RXACT_Msk));
	// Wait for DMA
	while (STREAM->CR & DMA_SxCR_EN_Msk);

	// Clear SDMMC flags
	MMC->ICR = SDMMC_ICR_DBCKENDC_Msk | SDMMC_ICR_DATAENDC_Msk |
			SDMMC_ICR_TXUNDERRC_Msk | SDMMC_ICR_RXOVERRC_Msk;
	// Clear DMA interrupts
	DMA2->LIFCR = DMA_LIFCR_CTCIF3_Msk | DMA_LIFCR_CHTIF3_Msk |
			DMA_LIFCR_CTEIF3_Msk | DMA_LIFCR_CDMEIF3_Msk |
			DMA_LIFCR_CFEIF3_Msk;

	// Set memory address
	STREAM->M0AR = (uint32_t)p;
	// Enable DMA stream
	STREAM->CR |= DMA_SxCR_EN_Msk;

	// Set transfer length
	MMC->DLEN = count * 512ul;
	// Start data transfer
	MMC->DCTRL = ((status & MMCRead) ? SDMMC_DCTRL_DTDIR_Msk : 0) |
			SDMMC_DCTRL_DMAEN_Msk | SDMMC_DCTRL_DTEN_Msk |
			(9ul << SDMMC_DCTRL_DBLOCKSIZE_Pos);

	// Statistics
	blocks += count;
	return count;
}

static int32_t mmc_busy()
{
	if (MMC->STA & (SDMMC_STA_TXACT_Msk | SDMMC_STA_RXACT_Msk))
		// Return as soon as FIFO become empty for performance
		return MMC->FIFOCNT;
	else if (status & (MMCWrite | MMCRead))
		// TX/RX not active yet, or already finished
		return !(MMC->STA & SDMMC_STA_DATAEND_Msk);
	else
		return 0;
}

static int32_t mmc_transferred()
{
	return 0xffffu - STREAM->NDTR;
}

static uint32_t mmc_stop()
{
	if (!(status & (MMCWrite | MMCRead))) {
		dbgbkpt();
		return 1;
	}

	// Wait for data block transfer
	while (MMC->STA & (SDMMC_STA_TXACT_Msk | SDMMC_STA_RXACT_Msk));
	MMC->DCTRL = 0;
	// Wait for DMA
	while (STREAM->CR & DMA_SxCR_EN_Msk);

	// Clear SDMMC flags
	MMC->ICR = SDMMC_ICR_DBCKENDC_Msk | SDMMC_ICR_DATAENDC_Msk |
			SDMMC_ICR_DTIMEOUTC_Msk | SDMMC_ICR_DCRCFAILC_Msk |
			SDMMC_ICR_TXUNDERRC_Msk | SDMMC_ICR_RXOVERRC_Msk;
	// Clear DMA interrupts
	DMA2->LIFCR = DMA_LIFCR_CTCIF3_Msk | DMA_LIFCR_CHTIF3_Msk |
			DMA_LIFCR_CTEIF3_Msk | DMA_LIFCR_CDMEIF3_Msk |
			DMA_LIFCR_CFEIF3_Msk;

	// Stop transmission for multi-block operation
	if (status & MMCMulti) {
		uint32_t resp, stat;
		resp = mmc_command(STOP_TRANSMISSION, 0, &stat);
		if (!(stat & SDMMC_STA_CMDREND_Msk)) {
			status = MMCIdle;
			return 2;
		}
	}

	status = MMCIdle;
	return 0;
}

static uint32_t mmc_write_block(const void *p, uint32_t start, uint32_t count)
{
	if (mmc_write_start(start, count) != count)
		return 0;
	if (mmc_data(p, count) != count)
		return 0;
	if (mmc_stop() != 0)
		return 0;
	return count;
}

static uint32_t mmc_read_block(void *p, uint32_t start, uint32_t count)
{
	if (mmc_read_prepare() != 0)
		return 0;
	if (mmc_data(p, count) != count)
		return 0;
	if (mmc_read_start(start, count) != count)
		return 0;
	if (mmc_stop() != 0)
		return 0;
	return count;
}

/* FatFs interface functions */

DSTATUS mmc_disk_init()
{
	if (!stat)
		return stat;

	if (stat & STA_NOINIT)
		mmc_base_init();

	if (mmc_cd() & STA_NODISK)
		return stat;

	mmc_reset(STA_ERROR);

	// Initialise SDMMC module
	RCC->APB2ENR |= RCC_APB2ENR_SDMMC1EN_Msk;

	// Disable interrupts
	MMC->MASK = 0ul;
	// 1-bit bus, clock enable, 400kHz
	MMC->CLKCR = SDMMC_CLKCR_CLKEN_Msk | (CEIL(clkSDMMC1(), 400000ul) - 2ul);
	// Start card clock
	MMC->POWER = 0b11ul << SDMMC_POWER_PWRCTRL_Pos;
	systick_delay(2);

	// Device may use up to 74 clocks for preparation
	for (uint32_t i = 10; i != 0; i--)
		// Reset card
		mmc_command(GO_IDLE_STATE, 0, 0);

	// Verify operation (2.7-3.6V, check pattern 0xa5)
	uint32_t sta, arg = (0b0001ul << 8u) | (0xa5 << 0u);
	uint32_t resp = mmc_command(SEND_IF_COND, arg, &sta);
	if (!(sta & SDMMC_STA_CMDREND_Msk)) {
		// Voltage mismatch or Ver.1 or not SD
		return stat;
	}

	// SD version 2.0 or later
	if (resp != arg) {
		// Unusable card
		dbgbkpt();
		return stat;
	}

	// Inquiry OCR
	uint32_t ocr = mmc_app_command(SD_SEND_OP_COND, 0, 0, 0);
	if (~ocr & (0b11 << 20u)) {
		// 3.2~3.4V unsupported
		dbgbkpt();
		return stat;
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
		return stat;
	}

	// Read CID register
	mmc_command(ALL_SEND_CID, 0, &sta);
	if (!(sta & SDMMC_STA_CTIMEOUT_Msk)) {
		// More than 1 card?
		dbgbkpt();
	}

	// Initialised, switch to high-speed clock
	// The last busy in any write operation up to 500ms
	uint32_t clk;
	if (!ccs) {	// SDSC, up to 25MHz
		clk = CEIL(clkSDMMC1(), 25000000ul) - 2ul;
		MMC->DTIMER = (clkSDMMC1() / (clk + 2ul)) / 2ul;
	} else {	// SDHC/SDXC, up to 50MHz
		clk = SDMMC_CLKCR_BYPASS_Msk;
		MMC->DTIMER = clkSDMMC1() / 2ul;
	}
	// Flow control, 4-bit bus, power saving, clock enable
	MMC->CLKCR = SDMMC_CLKCR_HWFC_EN_Msk | (0b01ul << SDMMC_CLKCR_WIDBUS_Pos) |
			SDMMC_CLKCR_PWRSAV_Msk | SDMMC_CLKCR_CLKEN_Msk | clk;

	// Read CSD register
	mmc_command(SEND_CSD, rca, &sta);
	if (!(sta & SDMMC_STA_CMDREND_Msk))
		return stat;

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
	printf(ESC_INFO "[MMC] Card capacity: " ESC_DATA "%lu"
	       ESC_INFO " MiB\n", (uint32_t)(capacity / 2ull / 1024ull));

	// Select card
	resp = mmc_command(SELECT_CARD, rca, &sta);
	if (!(sta & SDMMC_STA_CMDREND_Msk) || (resp & STAT_ERROR)) {
		dbgbkpt();
		return stat;
	}

	// Set bus width (4 bits bus)
	resp = mmc_app_command(SET_BUS_WIDTH, rca, 0b10, &sta);
	if (!(sta & SDMMC_STA_CMDREND_Msk) || (resp & STAT_ERROR)) {
		dbgbkpt();
		return stat;
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
		return stat;
	}

	// Clear SDMMC flags
	MMC->ICR = SDMMC_ICR_DBCKENDC_Msk | SDMMC_ICR_DATAENDC_Msk |
			SDMMC_ICR_DTIMEOUTC_Msk | SDMMC_ICR_DCRCFAILC_Msk |
			SDMMC_ICR_TXUNDERRC_Msk | SDMMC_ICR_RXOVERRC_Msk;

	mmc_reset(0);
	return stat;
}

DSTATUS mmc_disk_status()
{
	if ((stat & STA_NOINIT) || !(mmc_cd() & STA_NODISK))
		mmc_disk_init();
	if (!(stat & STA_NODISK) && mmc_ping())
		mmc_reset(STA_ERROR);
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

uint8_t scsi_buf[64 * 1024] ALIGN(32), *scsi_ptr;

uint8_t scsi_sense(scsi_t *scsi, uint8_t *sense, uint8_t *asc, uint8_t *ascq)
{
	// Update status
	mmc_disk_status();
	// Construct SCSI sense code
	if (stat & STA_NODISK) {
		*sense = NOT_READY;
		// 3A/00  DZT ROM  BK    MEDIUM NOT PRESENT
		*asc = 0x3a;
		*ascq = 0x00;
		return CHECK_CONDITION;
	} else if (stat & STA_ERROR) {
		*sense = NOT_READY;
		// 30/00  DZT ROM  BK    INCOMPATIBLE MEDIUM INSTALLED
		*asc = 0x30;
		*ascq = 0x00;
		return CHECK_CONDITION;
	} else if (stat) {
		*sense = NOT_READY;
		// 04/00  DZTPROMAEBKVF  LOGICAL UNIT NOT READY, CAUSE NOT REPORTABLE
		*asc = 0x04;
		*ascq = 0x00;
		return CHECK_CONDITION;
	}
	*sense = NO_SENSE;
	// 00/00  DZTPROMAEBKVF  NO ADDITIONAL SENSE INFORMATION
	*asc = 0x00;
	*ascq = 0x00;
	return GOOD;
}

uint32_t scsi_capacity(scsi_t *scsi, uint32_t *lbnum, uint32_t *lbsize)
{
	*lbnum = stat ? 0ul : capacity;
	*lbsize = 512ul;
	return 0;
}

uint32_t scsi_read_start(scsi_t *scsi, uint32_t offset, uint32_t size)
{
	// Check buffer overflow
	if (size * 512ul > sizeof(scsi_buf)) {
		dbgbkpt();
		return 0;
	}

	// Invalidate data cache for DMA operation
	SCB_InvalidateDCache_by_Addr((void *)scsi_buf, size * 512ul);

	if (mmc_read_prepare() != 0)
		return 0;
	if (mmc_data(scsi_buf, size) != size)
		return 0;
	if (mmc_read_start(offset, size) != size)
		return 0;
	scsi_ptr = scsi_buf;
	return size;
}

int32_t scsi_read_available(scsi_t *scsi)
{
	// Update status
	if (mmc_disk_status())
		return -1;

	int32_t size = mmc_transferred();
	if (size < 0)
		return size;
	return (size * 4ul - (scsi_ptr - scsi_buf)) / 512ul;
}

void *scsi_read_data(scsi_t *scsi, uint32_t *length)
{
	// Check buffer overflow
	if (*length * 512ul + (scsi_ptr - scsi_buf) > sizeof(scsi_buf)) {
		dbgbkpt();
		*length = 0;
		return 0;
	}

	void *p = scsi_ptr;
	scsi_ptr += *length * 512ul / sizeof(*scsi_ptr);
	return p;
}

uint32_t scsi_read_stop(scsi_t *scsi)
{
	return mmc_stop();
}

uint32_t scsi_write_start(scsi_t *scsi, uint32_t offset, uint32_t size)
{
	return mmc_write_start(offset, size);
}

uint32_t scsi_write_data(scsi_t *scsi, uint32_t length, const void *p)
{
	return mmc_data(p, length);
}

int32_t scsi_write_busy(scsi_t *scsi)
{
	// Update status
	if (mmc_disk_status())
		return -1;

	return mmc_busy();
}

uint32_t scsi_write_stop(scsi_t *scsi)
{
	return mmc_stop();
}
