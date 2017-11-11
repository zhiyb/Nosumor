#include <macros.h>
#include <debug.h>
#include "scsi.h"
#include "scsi_defs.h"

#define SCSI_BUF_SIZE	64u

// SCSI device info
typedef struct scsi_t {
	uint8_t buf[SCSI_BUF_SIZE];
} scsi_t;

scsi_t *scsi_init()
{
	return 0;
}

static scsi_ret_t sense(scsi_t *scsi, uint8_t status,
			uint8_t sense, uint8_t asc, uint8_t ascq)
{
	panic();
}

static scsi_ret_t unimplemented(scsi_t *scsi)
{
	dbgprintf("[SCSI]Unimplemented.");
	dbgbkpt();
	// 00/00  DZTPROMAEBKVF  NO ADDITIONAL SENSE INFORMATION
	return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x00, 0x00);
}

static scsi_ret_t inquiry(scsi_t *scsi, cmd_INQUIRY_t *cmd)
{
	if (cmd->info & CMD_INQUIRY_EVPD_Msk) {
		// Vital product data information
		if (cmd->info & CMD_INQUIRY_CMDDT_Msk) {
			dbgprintf("[SCSI]INQUIRY, Vital, Invalid.");
			dbgbkpt();
			// 24h/00h  DZTPROMAEBKVF  INVALID FIELD IN CDB
			return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00);
		}
		if (cmd->length < 4) {
			dbgprintf("[SCSI]INQUIRY, Vital, Invalid length.");
			dbgbkpt();
			// 24h/00h  DZTPROMAEBKVF  INVALID FIELD IN CDB
			return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00);
		}
		dbgbkpt();
		return unimplemented(scsi);
	} else {
		// Standard INQUIRY data
		if (cmd->page != 0) {
			dbgprintf("[SCSI]INQUIRY, Standard, Invalid.");
			dbgbkpt();
			// 24h/00h  DZTPROMAEBKVF  INVALID FIELD IN CDB
			return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00);
		}
		if (cmd->length < 5) {
			dbgprintf("[SCSI]INQUIRY, Standard, Invalid length.");
			dbgbkpt();
			// 24h/00h  DZTPROMAEBKVF  INVALID FIELD IN CDB
			return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00);
		}
		dbgbkpt();
		return unimplemented(scsi);
	}
}

scsi_ret_t scsi_cmd(scsi_t *scsi, const void *pdata, uint8_t size)
{
	cmd_t *cmd = (cmd_t *)pdata;
	if (size != cmd_size[cmd->op]) {
		dbgprintf("[SCSI]Invalid cmd size.");
		dbgbkpt();
		// 00/00  DZTPROMAEBKVF  NO ADDITIONAL SENSE INFORMATION
		return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x00, 0x00);
	}
	switch (cmd->op) {
	case INQUIRY:
		return inquiry(scsi, (cmd_INQUIRY_t *)cmd);
	}
	return unimplemented(scsi);
}
