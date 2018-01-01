#include <string.h>
#include <macros.h>
#include <debug.h>
#include <vendor_defs.h>
#include "scsi.h"
#include "scsi_defs.h"

#define SCSI_BUF_SIZE	64u

// SCSI device info
typedef struct scsi_t {
	uint8_t buf[SCSI_BUF_SIZE];
} scsi_t;

scsi_t *scsi_init()
{
	scsi_t *scsi = (scsi_t *)malloc(sizeof(scsi_t));
	if (!scsi)
		panic();
	return scsi;
}

static scsi_ret_t sense(scsi_t *scsi, uint8_t status,
			uint8_t sense, uint8_t asc, uint8_t ascq)
{
	uint8_t response = 0;
	if (asc == 0x29)
		response = 0x70;
	// MODE PARAMETERS CHANGED
	else if (asc == 0x2a && ascq == 0x01)
		response = 0x70;

	if (sense == CHECK_CONDITION)
		;
	panic();
}

static scsi_ret_t unimplemented(scsi_t *scsi)
{
	dbgprintf("[SCSI]Unimplemented.");
	dbgbkpt();
	// 00/00  DZTPROMAEBKVF  NO ADDITIONAL SENSE INFORMATION
	return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x00, 0x00);
}

static scsi_ret_t inquiry_standard(scsi_t *scsi, cmd_INQUIRY_t *cmd)
{
	data_INQUIRY_STANDARD_t *data = (data_INQUIRY_STANDARD_t *)scsi->buf;
	data->peripheral = PERIPHERAL_TYPE;
	// RMB(0b): Not removable
	data->peripheral_flags = 0;
	// Version(0h)
	data->version = 0x00;
	// NORMACA(0b), HISUP(0b), Format(2h)
	data->response = 0x02;
	// Remaining bytes
	data->additional = 0x20;
	// SCCS, ACC, TPGS[2], 3PC, Reserved[2], PROTECT
	data->flags[0] = 0x00;
	// BQUE (obs), ENCSERV, VS, MULTIP, MCHNGR, Obs, Obs, ADDR16(a)
	data->flags[1] = 0x80;
	// Obs, Obs, WBUS16(a), SYNC(a), LINKED (obs), Obs, CMDQUE, VS
	data->flags[2] = 0x00;
	memcpy(data->vendor, "ST MICRO", 8);
	strcpy((void *)data->product, PRODUCT_NAME);
	memcpy(data->revision, SW_VERSION_STR, 4);
	scsi_ret_t ret = {.p = data, .length = 4 + data->additional};
	return ret;
}

static scsi_ret_t inquiry_vital(scsi_t *scsi, cmd_INQUIRY_t *cmd)
{
	data_INQUIRY_VITAL_t *data = (data_INQUIRY_VITAL_t *)scsi->buf;
	data->peripheral = PERIPHERAL_TYPE;
	data->page = cmd->page;
	switch(cmd->page) {
	// Unit Serial Number page
	case 0x80:
		data->length = 0x08;
		memcpy(data->payload, SW_VERSION_STR, 4);
		break;
	default:
		dbgbkpt();
		// 24h/00h  DZTPROMAEBKVF  INVALID FIELD IN CDB
		return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00);
	}

	scsi_ret_t ret = {data, data->length, 0};
	return ret;
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
		return inquiry_vital(scsi, cmd);
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
		return inquiry_standard(scsi, cmd);
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
