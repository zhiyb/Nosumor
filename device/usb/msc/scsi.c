#include <string.h>
#include <stdlib.h>
#include <macros.h>
#include <debug.h>
#include <vendor_defs.h>
#include "scsi.h"
#include "scsi_defs.h"

#define SCSI_BUF_SIZE	64u
#define RAM_DISK_BLOCK	512u
#define RAM_DISK_SIZE	(64u * 1024)
#define RAM_DISK_BLOCKS	(RAM_DISK_SIZE / RAM_DISK_BLOCK)

// SCSI device info
typedef struct scsi_t {
	uint8_t buf[SCSI_BUF_SIZE] ALIGN(4);
	struct {
		// Control mode page (0ah), D_SENSE
		uint8_t type;
		uint8_t sense;
		uint8_t asc;
		uint8_t ascq;
	} sense;
} scsi_t;

uint8_t ram_disk[RAM_DISK_SIZE] ALIGN(4);

scsi_t *scsi_init()
{
	scsi_t *scsi = (scsi_t *)calloc(1u, sizeof(scsi_t));
	if (!scsi)
		panic();
	return scsi;
}

static scsi_ret_t sense_fixed(scsi_t *scsi, uint8_t response, uint8_t status,
			uint8_t sense, uint8_t asc, uint8_t ascq)
{
	sense_fixed_t *data = (sense_fixed_t *)scsi->buf;
	// [7]: VALID; [6:0]: Response code
	data->response = 0x80 | response;
	// [7:4]: FILEMARK, EOM, ILI, RESERVED; [3:0]: Sense Key
	data->key = sense;
	data->additional = 7;
	data->sense = asc;
	data->qualifier = ascq;

	scsi_ret_t ret = {data, 7 + data->additional, Failure};
	return ret;
}

static scsi_ret_t sense(scsi_t *scsi, uint8_t status,
			uint8_t sense, uint8_t asc, uint8_t ascq)
{
	uint8_t response;
	if (status == CHECK_CONDITION)
		// Current errors
		response = 0x70;
	else
		// Deferred errors
		response = 0x71;

	// Descriptor format
	if (scsi->sense.type)
		response += 2;

	// Special modes
	if (asc == 0x29)
		response = 0x70;
	// MODE PARAMETERS CHANGED
	else if (asc == 0x2a && ascq == 0x01)
		response = 0x70;

	if (response != 0x70)
		panic();
	else
		return sense_fixed(scsi, response, status, sense, asc, ascq);
}

static scsi_ret_t unimplemented(scsi_t *scsi)
{
	dbgprintf("[SCSI]Unimplemented.");
	dbgbkpt();
	// 00/00  DZTPROMAEBKVF  NO ADDITIONAL SENSE INFORMATION
	return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x00, 0x00);
}

static scsi_ret_t test_unit_ready(scsi_t *scsi, cmd_TEST_UNIT_READY_t *cmd)
{
	scsi_ret_t ret = {0, 0, Good};
	return ret;
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

	scsi_ret_t ret = {data, 4 + data->additional, Good};
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
		// 24/00  DZTPROMAEBKVF  INVALID FIELD IN CDB
		return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00);
	}

	scsi_ret_t ret = {data, data->length, Good};
	// Endianness conversion
	data->length = __REV16(data->length);
	return ret;
}

static scsi_ret_t inquiry(scsi_t *scsi, cmd_INQUIRY_t *cmd)
{
	// Endianness conversion
	cmd->length = __REV16(cmd->length);

	if (cmd->info & CMD_INQUIRY_EVPD_Msk) {
		// Vital product data information
		if (cmd->info & CMD_INQUIRY_CMDDT_Msk) {
			dbgprintf("[SCSI]INQUIRY, Vital, Invalid.");
			dbgbkpt();
			// 24/00  DZTPROMAEBKVF  INVALID FIELD IN CDB
			return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00);
		}
		if (cmd->length < 4) {
			dbgprintf("[SCSI]INQUIRY, Vital, Invalid length.");
			dbgbkpt();
			// 24/00  DZTPROMAEBKVF  INVALID FIELD IN CDB
			return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00);
		}
		return inquiry_vital(scsi, cmd);
	} else {
		// Standard INQUIRY data
		if (cmd->page != 0) {
			dbgprintf("[SCSI]INQUIRY, Standard, Invalid.");
			dbgbkpt();
			// 24/00  DZTPROMAEBKVF  INVALID FIELD IN CDB
			return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00);
		}
		if (cmd->length < 5) {
			dbgprintf("[SCSI]INQUIRY, Standard, Invalid length.");
			dbgbkpt();
			// 24/00  DZTPROMAEBKVF  INVALID FIELD IN CDB
			return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00);
		}
		return inquiry_standard(scsi, cmd);
	}
}

static scsi_ret_t request_sense(scsi_t *scsi, cmd_REQUEST_SENSE_t *cmd)
{
	scsi_ret_t ret;
	if (cmd->desc)
		ret = unimplemented(scsi);
	else
		// 00/00  DZTPROMAEBKVF  NO ADDITIONAL SENSE INFORMATION
		ret = sense_fixed(scsi, 0x70, GOOD, NO_SENSE, 0x00, 0x00);

	ret.state = Good;
	if (ret.length > cmd->length)
		ret.length = cmd->length;
	return ret;
}

static scsi_ret_t read_capacity_10(scsi_t *scsi, cmd_READ_CAPACITY_10_t *cmd)
{
	// Endianness conversion
	cmd->lbaddr = __REV(cmd->lbaddr);

	if (cmd->pmi) {
		return unimplemented(scsi);
	} else {
		if (cmd->lbaddr != 0)
			// 24/00  DZTPROMAEBKVF  INVALID FIELD IN CDB
			return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00);

		// TODO: Actual size
		data_READ_CAPACITY_10_t *data = (data_READ_CAPACITY_10_t *)scsi->buf;
		data->lbsize = RAM_DISK_BLOCK;
		data->lbaddr = RAM_DISK_BLOCKS - 1;

		scsi_ret_t ret = {data, 8, Good};
		// Endianness conversion
		data->lbaddr = __REV(data->lbaddr);
		data->lbsize = __REV(data->lbsize);
		return ret;
	}
}

static scsi_ret_t read_10(scsi_t *scsi, cmd_READ_10_t *cmd)
{
	// Endianness conversion
	cmd->lbaddr = __REV(cmd->lbaddr);
	cmd->length = __REV16(cmd->length);

	if (cmd->flags != 0)
		dbgbkpt();

	// Logical block address check
	if (cmd->lbaddr >= RAM_DISK_BLOCKS) {
		dbgbkpt();
		// 21/00  DZT RO   BK    LOGICAL BLOCK ADDRESS OUT OF RANGE
		return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x21, 0x00);
	}

	if (cmd->group != 0)
		dbgbkpt();

	// Transfer length check
	if (cmd->lbaddr + (cmd->length + RAM_DISK_BLOCKS - 1) / RAM_DISK_BLOCKS >= RAM_DISK_BLOCKS) {
		dbgbkpt();
		// 21/00  DZT RO   BK    LOGICAL BLOCK ADDRESS OUT OF RANGE
		return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x21, 0x00);
	}

	dbgprintf("[SCSI] Read %u blocks from %lu.\n", cmd->length, cmd->lbaddr);

	scsi_ret_t ret = {0, 0, Good};
	if (cmd->length == 0)
		return ret;

	ret.p = ram_disk;
	ret.length = cmd->length * RAM_DISK_BLOCK;
	return ret;
}

scsi_ret_t scsi_cmd(scsi_t *scsi, const void *pdata, uint8_t size)
{
	memset(scsi->buf, 0, sizeof(scsi->buf));

	// Unsupported commands
	cmd_t *cmd = (cmd_t *)pdata;
	switch (cmd->op) {
	case READ_FORMAT_CAPACITIES:
	case MODE_SENSE_6:
	case SYNCHRONIZE_CACHE_10:
	case START_STOP_UNIT:
		// 00/00  DZTPROMAEBKVF  NO ADDITIONAL SENSE INFORMATION
		return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x00, 0x00);
	}

	// Command size check
	if (size < cmd_size[cmd->op]) {
		dbgprintf("[SCSI] Invalid cmd size.");
		dbgbkpt();
		// 00/00  DZTPROMAEBKVF  NO ADDITIONAL SENSE INFORMATION
		return sense(scsi, CHECK_CONDITION, ILLEGAL_REQUEST, 0x00, 0x00);
	}

	// Command handling
	switch (cmd->op) {
	case TEST_UNIT_READY:
		return test_unit_ready(scsi, (cmd_TEST_UNIT_READY_t *)cmd);
	case INQUIRY:
		return inquiry(scsi, (cmd_INQUIRY_t *)cmd);
	case REQUEST_SENSE:
		return request_sense(scsi, (cmd_REQUEST_SENSE_t *)cmd);
	case READ_CAPACITY_10:
		return read_capacity_10(scsi, (cmd_READ_CAPACITY_10_t *)cmd);
	case READ_10:
		return read_10(scsi, (cmd_READ_10_t *)cmd);
	}
	return unimplemented(scsi);
}
