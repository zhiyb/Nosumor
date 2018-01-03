#ifndef MMC_DEFS_H
#define MMC_DEFS_H

#include <stm32f7xx.h>

// Response length
#define RESP_NONE	(0b00ul << SDMMC_CMD_WAITRESP_Pos)
#define RESP_SHORT	(0b01ul << SDMMC_CMD_WAITRESP_Pos)
#define RESP_LONG	(0b11ul << SDMMC_CMD_WAITRESP_Pos)

// Standard commands
#define GO_IDLE_STATE		(0u | RESP_NONE)	// -
#define ALL_SEND_CID		(2u | RESP_LONG)	// R2
#define SEND_RELATIVE_ADDR	(3u | RESP_SHORT)	// R6
#define SELECT_CARD		(7u | RESP_SHORT)	// R1b
#define SEND_IF_COND		(8u | RESP_SHORT)	// R7
#define SEND_CSD		(9u | RESP_LONG)	// R2
#define SEND_CID		(10u | RESP_LONG)	// R2
#define VOLTAGE_SWITCH		(11u | RESP_SHORT)	// R1
#define STOP_TRANSMISSION	(12u | RESP_SHORT)	// R1b
#define SEND_STATUS		(13u | RESP_SHORT)	// R1
#define SET_BLOCKLEN		(16u | RESP_SHORT)	// R1
#define READ_SINGLE_BLOCK	(17u | RESP_SHORT)	// R1
#define READ_MULTIPLE_BLOCK	(18u | RESP_SHORT)	// R1
#define SEND_TUNING_BLOCK	(19u | RESP_SHORT)	// R1
#define SET_BLOCK_COUNT		(23u | RESP_SHORT)	// R1
#define WRITE_BLOCK		(24u | RESP_SHORT)	// R1
#define WRITE_MULTIPLE_BLOCK	(25u | RESP_SHORT)	// R1
#define LOCK_UNLOCK		(42u | RESP_SHORT)	// R1
#define APP_CMD			(55u | RESP_SHORT)	// R1

// Application specific commands
#define SET_BUS_WIDTH		(6u | RESP_SHORT)	// R1
#define SEND_NUM_WR_BLOCKS	(22u | RESP_SHORT)	// R1
#define SET_WR_BLK_ERASE_COUNT	(23u | RESP_SHORT)	// R1
#define SD_SEND_OP_COND		(41u | RESP_SHORT)	// R3

// Card status
#define STAT_OUT_OF_RANGE	(1ul << 31u)
#define STAT_ADDRESS_ERROR	(1ul << 30u)
#define STAT_BLOCK_LEN_ERROR	(1ul << 29u)
#define STAT_ERASE_SEQ_ERROR	(1ul << 28u)
#define STAT_ERASE_PARAM	(1ul << 27u)
#define STAT_WP_VIOLATION	(1ul << 26u)
#define STAT_CARD_IS_LOCKED	(1ul << 25u)
#define STAT_LOCK_UNLOCK_FAILED	(1ul << 24u)
#define STAT_COM_CRC_ERROR	(1ul << 23u)
#define STAT_ILLEGAL_COMMAND	(1ul << 22u)
#define STAT_CARD_ECC_FAILED	(1ul << 21u)
#define STAT_CC_ERROR		(1ul << 20u)
#define STAT_ERROR		(1ul << 19u)
#define STAT_CSD_OVERWRITE	(1ul << 16u)
#define STAT_WP_ERASE_SKIP	(1ul << 15u)
#define STAT_CARD_ECC_DISABLED	(1ul << 14u)
#define STAT_ERASE_RESET	(1ul << 13u)
#define STAT_CURRENT_STATE_Pos	(9u)
#define STAT_CURRENT_STATE_Msk	(0xful << STAT_CURRENT_STATE_Pos)
#define STAT_READY_FOR_DATA	(1ul << 8u)
#define STAT_FX_EVENT		(1ul << 6u)
#define STAT_APP_CMD		(1ul << 5u)
#define STAT_AKE_SEQ_ERROR	(1ul << 3u)
// R6: RCA
#define STAT_RCA_Pos		(16u)
#define STAT_RCA_Msk		(0xfffful << STAT_RCA_Pos)

// Operation conditions register
#define OCR_S18A	(1ul << 24u)
#define OCR_UHS_II	(1ul << 29u)
#define OCR_CCS		(1ul << 30u)
#define OCR_BUSY	(1ul << 31u)
// Host capacity
#define OCR_XPC		(1ul << 28u)
#define OCR_HCS		(1ul << 30u)

#endif // MMC_DEFS_H
