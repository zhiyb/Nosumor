#include <string.h>
#include <stm32f7xx.h>
#include "mmc.h"
#include "mmc_defs.h"
#include "../systick.h"
#include "../clocks.h"
#include "../macros.h"
#include "../debug.h"

#define MMC	SDMMC1

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
static uint32_t ccs = 0, capacity = 0;

// Send command to card, with optional status output
// Returns short response
static uint32_t mmc_command(uint32_t cmd, uint32_t arg, uint32_t *stat);
// RCA: [31:16] RCA, [15:0] stuff bits
static uint32_t mmc_app_command(uint32_t cmd, uint32_t rca, uint32_t arg, uint32_t *stat);

static inline void mmc_gpio_init()
{
	// Enable IO compensation cell
	if (!(SYSCFG->CMPCR & SYSCFG_CMPCR_READY)) {
		RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
		SYSCFG->CMPCR |= SYSCFG_CMPCR_CMP_PD;
	}
	// Initialise GPIOs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN_Msk |
			RCC_AHB1ENR_GPIOCEN_Msk | RCC_AHB1ENR_GPIODEN_Msk;
	// PA15, CD, input
	GPIO_MODER(GPIOA, 15, 0b00);
	GPIO_PUPDR(GPIOA, 15, GPIO_PUPDR_UP);
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
}

DSTATUS mmc_disk_init()
{
	if (stat & STA_NOINIT) {
		mmc_gpio_init();
		// Initialise SDMMC module
		RCC->APB2ENR |= RCC_APB2ENR_SDMMC1EN_Msk;
	}

	// Switch opens when card inserted
	if (GPIOA->IDR & (1ul << 15))
		stat = STA_ERROR;
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
	uint32_t rca = mmc_command(SEND_RELATIVE_ADDR, 0, &sta);
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
		dbgprintf(ESC_CYAN "size: %lu, mult: %lu, len: %lu\n", size, mult, len);
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

	// Set data access configurations
	// TODO: Read optimal timeout values from CSD register
	MMC->DTIMER = clkSDMMC1() / 10ul;

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
	if (count == 1u) {
		uint32_t stat, resp;
		resp = mmc_command(READ_SINGLE_BLOCK, ccs ? sector : sector * 512u, &stat);
		if (!(stat & SDMMC_STA_CMDREND_Msk) || (resp & STAT_ERROR)) {
			dbgbkpt();
			return RES_ERROR;
		}
		MMC->DLEN = 512ul;
		MMC->DCTRL = SDMMC_DCTRL_SDIOEN_Msk | SDMMC_DCTRL_RWSTART_Msk |
				SDMMC_DCTRL_DTDIR_Msk | SDMMC_DCTRL_DTEN_Msk |
				(9ul << SDMMC_DCTRL_DBLOCKSIZE_Pos);
		count = 512u / 4u;
		while (count--) {
			while (!(MMC->STA & SDMMC_STA_RXDAVL_Msk));
			*(uint32_t *)buff = MMC->FIFO;
			buff += 4u;
		}
		return RES_OK;
	}
	dbgbkpt();
	return RES_ERROR;
}

static uint32_t mmc_command(uint32_t cmd, uint32_t arg, uint32_t *stat)
{
	// Send CMD8
	MMC->ICR = SDMMC_ICR_CMDSENTC_Msk | SDMMC_ICR_CMDRENDC_Msk |
			SDMMC_ICR_CCRCFAILC_Msk | SDMMC_ICR_CTIMEOUTC_Msk;
	MMC->ARG = arg;
	MMC->CMD = cmd | SDMMC_CMD_CPSMEN_Msk;
	while (!(MMC->STA & SDMMC_STA_CMDACT_Msk));

	if (!(cmd & SDMMC_CMD_WAITRESP_0)) {
		// No response expected
		while (!(MMC->STA & SDMMC_STA_CMDSENT_Msk));
		if (stat)
			*stat = MMC->STA;
		return MMC->RESPCMD;
	}

	while (!(MMC->STA & (SDMMC_STA_CMDREND_Msk |
			     SDMMC_STA_CCRCFAIL_Msk | SDMMC_STA_CTIMEOUT_Msk)));
	if (stat)
		*stat = MMC->STA;
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
