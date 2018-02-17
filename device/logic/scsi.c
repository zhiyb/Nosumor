#include <string.h>
#include <stdlib.h>
#include <macros.h>
#include <debug.h>
#include <api_defs.h>
#include "scsi.h"
#include "scsi_defs.h"

#define SCSI_BUF_SIZE	64u

typedef struct sense_t {
	uint8_t status;
	uint8_t sense;
	uint8_t asc;
	uint8_t ascq;
} sense_t;

// SCSI device info
typedef struct scsi_t {
	// Interface handler functions
	const scsi_handlers_t *h;
	// Interface private data
	void *p;

	// SCSI state (next packet)
	scsi_state_t state;
	// Remaining transfer length (read/write)
	uint32_t length;
	// Data output buffer
	uint8_t buf[SCSI_BUF_SIZE] ALIGN(4);

	// Control mode page (0ah), D_SENSE
	uint8_t sense_type;
	sense_t sense;
} scsi_t;

scsi_t *scsi_init(const scsi_if_t iface)
{
	scsi_t *scsi = (scsi_t *)calloc(1u, sizeof(scsi_t));
	if (!scsi) {
		dbgbkpt();
		return 0;
	}
	scsi->h = iface.handlers;
	scsi->p = iface.data;
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

	scsi_ret_t ret = {data, 7 + data->additional, scsi->state};
	return ret;
}

static scsi_ret_t unimplemented(scsi_t *scsi)
{
	dbgprintf(ESC_ERROR "[SCSI] Unimplemented\n");
	dbgbkpt();
	// 00/00  DZTPROMAEBKVF  NO ADDITIONAL SENSE INFORMATION
	scsi->sense = (sense_t){CHECK_CONDITION, ILLEGAL_REQUEST, 0x00, 0x00};
	return (scsi_ret_t){0, 0, SCSIFailure};
}

static scsi_ret_t test_unit_ready(scsi_t *scsi, cmd_TEST_UNIT_READY_t *cmd)
{
	dbgprintf(ESC_DEBUG "[SCSI] Test unit ready\n");

	// Check media status
	scsi->sense.status = scsi->h->sense(scsi->p, &scsi->sense.sense,
					    &scsi->sense.asc, &scsi->sense.ascq);
	if (scsi->sense.status != GOOD) {
		dbgprintf(ESC_ERROR "[SCSI] Unit not ready\n");
		return (scsi_ret_t){0, 0, SCSIFailure};
	}

	scsi_ret_t ret = {0, 0, SCSIGood};
	return ret;
}

static scsi_ret_t inquiry_standard(scsi_t *scsi, cmd_INQUIRY_t *cmd)
{
	// Retrieve capacity information
	uint32_t lbnum, lbsize;
	scsi->h->capacity(scsi->p, &lbnum, &lbsize);

	dbgprintf(ESC_DEBUG "[SCSI] Standard INQUIRY\n");
	data_INQUIRY_STANDARD_t *data = (data_INQUIRY_STANDARD_t *)scsi->buf;
	data->peripheral = PERIPHERAL_TYPE;
	if (lbnum == 0)	// Device not connected
		data->peripheral |= 0x20;
	// RMB: Removable
	data->peripheral_flags = scsi->h->type & 0x80;
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
	strncpy((void *)data->vendor, PRODUCT, sizeof(data->vendor));
	strncpy((void *)data->product, scsi->h->name(scsi->p), sizeof(data->product));
	strncpy((void *)data->revision, SW_VERSION_STR, sizeof(data->revision));

	scsi_ret_t ret = {data, 4 + data->additional, SCSIGood};
	return ret;
}

static scsi_ret_t inquiry_vital(scsi_t *scsi, cmd_INQUIRY_t *cmd)
{
	// Retrieve capacity information
	uint32_t lbnum, lbsize;
	scsi->h->capacity(scsi->p, &lbnum, &lbsize);

	data_INQUIRY_VITAL_t *data = (data_INQUIRY_VITAL_t *)scsi->buf;
	data->peripheral = PERIPHERAL_TYPE;
	if (lbnum == 0)	// Device not connected
		data->peripheral |= 0x20;
	data->page = cmd->page;
	dbgprintf(ESC_DEBUG "[SCSI] Vital INQUIRY, page 0x%x\n", cmd->page);

	switch(cmd->page) {
	// Unit Serial Number page
	case 0x80:
		data->length = 0x08;
		memcpy(data->payload, SW_VERSION_STR, 4);
		break;
	default:
		dbgprintf(ESC_ERROR "[SCSI] Unimplemented VPD page\n");
		dbgbkpt();
		// 24/00  DZTPROMAEBKVF  INVALID FIELD IN CDB
		scsi->sense = (sense_t){CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00};
		return (scsi_ret_t){0, 0, SCSIFailure};
	}

	scsi_ret_t ret = {data, data->length, SCSIGood};
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
			dbgprintf(ESC_ERROR "[SCSI] Invalid vital INQUIRY\n");
			dbgbkpt();
			// 24/00  DZTPROMAEBKVF  INVALID FIELD IN CDB
			scsi->sense = (sense_t){CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00};
			return (scsi_ret_t){0, 0, SCSIFailure};
		}
		if (cmd->length < 4) {
			dbgprintf(ESC_ERROR "[SCSI] Invalid vital INQUIRY length\n");
			dbgbkpt();
			// 24/00  DZTPROMAEBKVF  INVALID FIELD IN CDB
			scsi->sense = (sense_t){CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00};
			return (scsi_ret_t){0, 0, SCSIFailure};
		}
		return inquiry_vital(scsi, cmd);
	} else {
		// Standard INQUIRY data
		if (cmd->page != 0) {
			dbgprintf(ESC_ERROR "[SCSI] Invalid standard INQUIRY\n");
			dbgbkpt();
			// 24/00  DZTPROMAEBKVF  INVALID FIELD IN CDB
			scsi->sense = (sense_t){CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00};
			return (scsi_ret_t){0, 0, SCSIFailure};
		}
		if (cmd->length < 5) {
			dbgprintf(ESC_ERROR "[SCSI] Invalid standard INQUIRY length\n");
			dbgbkpt();
			// 24/00  DZTPROMAEBKVF  INVALID FIELD IN CDB
			scsi->sense = (sense_t){CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00};
			return (scsi_ret_t){0, 0, SCSIFailure};
		}
		return inquiry_standard(scsi, cmd);
	}
}

static scsi_ret_t request_sense(scsi_t *scsi, cmd_REQUEST_SENSE_t *cmd)
{
	if (cmd->desc)
		return unimplemented(scsi);

	// Generate sense data
	scsi_ret_t ret = sense_fixed(scsi, 0x70, scsi->sense.status,
				     scsi->sense.sense,
				     scsi->sense.asc, scsi->sense.ascq);
	ret.state = SCSIGood;

	dbgprintf(ESC_DEBUG "[SCSI] Request sense: 0x%x, 0x%x, 0x%x/0x%x\n",
		  scsi->sense.status, scsi->sense.sense,
		  scsi->sense.asc, scsi->sense.ascq);

	// Clear sense data
	// 00/00  DZTPROMAEBKVF  NO ADDITIONAL SENSE INFORMATION
	scsi->sense = (sense_t){GOOD, NO_SENSE, 0x00, 0x00};

	if (ret.length > cmd->length)
		ret.length = cmd->length;
	return ret;
}

static scsi_ret_t read_capacity_10(scsi_t *scsi, cmd_READ_CAPACITY_10_t *cmd)
{
	// Endianness conversion
	cmd->lbaddr = __REV(cmd->lbaddr);

	dbgprintf(ESC_DEBUG "[SCSI] Read capacity\n");

	// Update sense data
	scsi_ret_t ret = test_unit_ready(scsi, 0);
	if (ret.state != SCSIGood)
		return ret;

	if (cmd->pmi) {
		return unimplemented(scsi);
	} else {
		if (cmd->lbaddr != 0) {
			dbgprintf(ESC_ERROR "[SCSI] Invalid LBA\n");
			dbgbkpt();
			// 24/00  DZTPROMAEBKVF  INVALID FIELD IN CDB
			scsi->sense = (sense_t){CHECK_CONDITION, ILLEGAL_REQUEST, 0x24, 0x00};
			return (scsi_ret_t){0, 0, SCSIFailure};
		}

		data_READ_CAPACITY_10_t *data = (data_READ_CAPACITY_10_t *)scsi->buf;
		scsi->h->capacity(scsi->p, &data->lbaddr, &data->lbsize);
		data->lbaddr--;

		scsi_ret_t ret = {data, 8, SCSIGood};
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

	// Retrieve capacity information
	uint32_t lbnum, lbsize;
	scsi->h->capacity(scsi->p, &lbnum, &lbsize);

	dbgprintf(ESC_READ "[SCSI] Read %u blocks from %lu\n", cmd->length, cmd->lbaddr);

	// Logical block address check
	if (cmd->lbaddr >= lbnum) {
		dbgprintf(ESC_ERROR "[SCSI] LBA out of range\n");
		// 21/00  DZT RO   BK    LOGICAL BLOCK ADDRESS OUT OF RANGE
		scsi->sense = (sense_t){CHECK_CONDITION, ILLEGAL_REQUEST, 0x21, 0x00};
		return (scsi_ret_t){0, 0, SCSIFailure};
	}

	if (cmd->group != 0)
		dbgbkpt();

	// Transfer length check
	if (cmd->lbaddr + cmd->length > lbnum * lbsize) {
		dbgprintf(ESC_ERROR "[SCSI] Invalid transfer length\n");
		// 21/00  DZT RO   BK    LOGICAL BLOCK ADDRESS OUT OF RANGE
		scsi->sense = (sense_t){CHECK_CONDITION, ILLEGAL_REQUEST, 0x21, 0x00};
		return (scsi_ret_t){0, 0, SCSIFailure};
	}

	if (cmd->length == 0)
		return (scsi_ret_t){0, 0, SCSIGood};

	if (scsi->h->read_start(scsi->p, cmd->lbaddr, cmd->length) != cmd->length) {
		dbgprintf(ESC_ERROR "[SCSI] initiating data read failed\n");
		// Update sense data
		test_unit_ready(scsi, 0);
		return (scsi_ret_t){0, 0, SCSIFailure};
	}

	scsi->length = cmd->length * lbsize;
	scsi->state = SCSIRead;
	return (scsi_ret_t){0, 0, SCSIRead};
}

static scsi_ret_t write_10(scsi_t *scsi, cmd_WRITE_10_t *cmd)
{
	// Endianness conversion
	cmd->lbaddr = __REV(cmd->lbaddr);
	cmd->length = __REV16(cmd->length);

	if (cmd->flags != 0)
		dbgbkpt();

	// Retrieve capacity information
	uint32_t lbnum, lbsize;
	scsi->h->capacity(scsi->p, &lbnum, &lbsize);

	dbgprintf(ESC_WRITE "[SCSI] Write %u blocks from %lu\n", cmd->length, cmd->lbaddr);

	// Logical block address check
	if (cmd->lbaddr >= lbnum) {
		dbgprintf(ESC_ERROR "[SCSI] LBA out of range\n");
		// 21/00  DZT RO   BK    LOGICAL BLOCK ADDRESS OUT OF RANGE
		scsi->sense = (sense_t){CHECK_CONDITION, ILLEGAL_REQUEST, 0x21, 0x00};
		return (scsi_ret_t){0, 0, SCSIFailure};
	}

	if (cmd->group != 0)
		dbgbkpt();

	// Transfer length check
	if (cmd->lbaddr + cmd->length > lbnum * lbsize) {
		dbgprintf(ESC_ERROR "[SCSI] Transfer length out of range\n");
		// 21/00  DZT RO   BK    LOGICAL BLOCK ADDRESS OUT OF RANGE
		scsi->sense = (sense_t){CHECK_CONDITION, ILLEGAL_REQUEST, 0x21, 0x00};
		return (scsi_ret_t){0, 0, SCSIFailure};
	}

	if (scsi->h->write_start(scsi->p, cmd->lbaddr, cmd->length) != cmd->length) {
		dbgprintf(ESC_ERROR "[SCSI] Initiating data write failed\n");
		// Update sense data
		test_unit_ready(scsi, 0);
		return (scsi_ret_t){0, 0, SCSIFailure};
	}

	scsi->length = cmd->length * lbsize;
	scsi->state = SCSIWrite;
	return (scsi_ret_t){0, 0, SCSIWrite};
}

scsi_ret_t scsi_cmd(scsi_t *scsi, const void *pdata, uint8_t size)
{
	cmd_t *cmd = (cmd_t *)pdata;
	memset(scsi->buf, 0, sizeof(scsi->buf));

	// Command size check
	uint8_t s = cmd_size[cmd->op];
	if (s == 0) {
		return unimplemented(scsi);
	} else if (size < cmd_size[cmd->op]) {
		dbgprintf(ESC_ERROR "[SCSI] Invalid cmd size\n");
		// 00/00  DZTPROMAEBKVF  NO ADDITIONAL SENSE INFORMATION
		scsi->sense = (sense_t){CHECK_CONDITION, ILLEGAL_REQUEST, 0x00, 0x00};
		return (scsi_ret_t){0, 0, SCSIFailure};
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
	case WRITE_10:
		return write_10(scsi, (cmd_WRITE_10_t *)cmd);
	default:
		dbgprintf(ESC_WARNING "[SCSI] Unsupported command: "
			  ESC_DATA "0x%02x\n", cmd->op);
		// 00/00  DZTPROMAEBKVF  NO ADDITIONAL SENSE INFORMATION
		scsi->sense = (sense_t){CHECK_CONDITION, ILLEGAL_REQUEST, 0x00, 0x00};
		return (scsi_ret_t){0, 0, SCSIFailure};
	}
}

scsi_state_t scsi_data(scsi_t *scsi, const void *pdata, uint32_t size)
{
	// Check SCSI state machine
	if ((scsi->state & ~SCSIBusy) != SCSIWrite) {
		dbgprintf(ESC_ERROR "[SCSI] State machine error: " ESC_DATA "%u\n", scsi->state);
		dbgbkpt();
		return SCSIFailure;
	}

	// Retrieve capacity information
	uint32_t lbnum, lbsize;
	scsi->h->capacity(scsi->p, &lbnum, &lbsize);

	// Check remaining data size
	if (scsi->length < size) {
		dbgbkpt();
		return SCSIFailure;
	}

	// Transfer data
	if (!(scsi->state & SCSIBusy)) {
		if (scsi->h->write_data(scsi->p, size, pdata) != size) {
			dbgprintf(ESC_ERROR "[SCSI] Write data failure\n");
			// Update sense data
			test_unit_ready(scsi, 0);
			// Reset SCSI state machine
			scsi->state = SCSIFailure;
			return scsi->state;
		}
	}

	// Check busy status
	int32_t busy = scsi->h->write_busy(scsi->p);
	if (busy < 0) {
		dbgprintf(ESC_ERROR "[SCSI] Write failure\n");
		// Update sense data
		test_unit_ready(scsi, 0);
		// Reset SCSI state machine
		scsi->state = SCSIFailure;
		return SCSIFailure;
	} else if (busy) {
		scsi->state = SCSIWrite | SCSIBusy;
	} else {
		// Update status
		scsi->length -= size;
		if (scsi->length) {
			scsi->state = SCSIWrite;
		} else {
			if (scsi->h->write_stop(scsi->p)) {
				dbgprintf(ESC_ERROR "[SCSI] Write failed\n");
				// Update sense data
				test_unit_ready(scsi, 0);
				// Reset SCSI state machine
				scsi->state = SCSIFailure;
			} else {
				scsi->state = SCSIGood;
			}
		}
	}
	return scsi->state;
}

scsi_state_t scsi_data_cplt(scsi_t *scsi, const void *pdata, uint32_t size)
{
	if (scsi->h->read_cplt)
		scsi->h->read_cplt(scsi->p, size, pdata);
	scsi->state &= ~SCSIBusy;
	return scsi->state;
}

scsi_ret_t scsi_process(scsi_t *scsi, uint32_t pktsize, uint32_t maxsize)
{
	// Avoid failure sort packet looping
	if (scsi->state == SCSIFailure)
		return (scsi_ret_t){0, 0, SCSIGood};
	// Only read events are currently processed here
	if ((scsi->state & ~SCSIBusy) != SCSIRead)
		return (scsi_ret_t){0, 0, scsi->state};

	// Retrieve capacity information
	uint32_t lbnum, lbsize;
	scsi->h->capacity(scsi->p, &lbnum, &lbsize);

	// Calculate packet size
	uint32_t size = maxsize < scsi->length ? maxsize : scsi->length;

	// Check again if insufficient buffering
	int32_t available = scsi->h->read_available(scsi->p);
	if (available < 0) {
		dbgprintf(ESC_ERROR "[SCSI] Read failure\n");
		// Update sense data
		test_unit_ready(scsi, 0);
		// Reset SCSI state machine
		scsi->state = SCSIFailure;
		return (scsi_ret_t){0, 0, scsi->state};
	} else if ((uint32_t)available < pktsize)
		return (scsi_ret_t){0, 0, scsi->state};

	// Read available data
	available &= ~(pktsize - 1);
	uint32_t length = available;
	void *p = scsi->h->read_data(scsi->p, &length);
	if (!p || length != (uint32_t)available) {
		dbgprintf(ESC_ERROR "[SCSI] Read data failure\n");
		// Update sense data
		test_unit_ready(scsi, 0);
		// Reset SCSI state machine
		scsi->state = SCSIFailure;
		return (scsi_ret_t){0, 0, scsi->state};
	}

	// Update status
	scsi->length -= length;
	if (!scsi->length) {
		if (scsi->h->read_stop(scsi->p)) {
			dbgprintf(ESC_ERROR "[SCSI] Read failed\n");
			// Update sense data
			test_unit_ready(scsi, 0);
			// Reset SCSI state machine
			scsi->state = SCSIFailure;
			return (scsi_ret_t){0, 0, scsi->state};
		} else {
			scsi->state = SCSIGood;
		}
	} else {
		scsi->state |= SCSIBusy;
	}
	return (scsi_ret_t){p, length, scsi->state};
}
