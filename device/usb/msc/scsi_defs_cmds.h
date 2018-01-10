#ifndef SCSI_DEFS_CMDS_H
#define SCSI_DEFS_CMDS_H

#include <stdint.h>
#include <macros.h>
#include "scsi_defs_ops.h"

/* Command types */

typedef struct PACKED cmd_t {
	uint8_t op;
	uint8_t payload[0];
} cmd_t;

// TEST UNIT READY
typedef struct PACKED cmd_TEST_UNIT_READY_t {
	uint8_t op;			// 00h
	uint8_t RESERVED1[4];
	uint8_t control;
} cmd_TEST_UNIT_READY_t;

// REQUEST SENSE
typedef struct PACKED cmd_REQUEST_SENSE_t {
	uint8_t op;			// 03h
	uint8_t desc;			// [0] DESC
	uint16_t RESERVED1;
	uint16_t length;		// Allocation Length
	uint8_t control;
} cmd_REQUEST_SENSE_t;

// INQUIRY
typedef struct PACKED cmd_INQUIRY_t {
	uint8_t op;			// 12h
	uint8_t info;			// [1] CMDDT (obsolete); [0] EVPD
	uint8_t page;
	uint16_t length;
	uint8_t control;
} cmd_INQUIRY_t;

#define CMD_INQUIRY_EVPD_Msk	0x01
#define CMD_INQUIRY_CMDDT_Msk	0x02

typedef struct PACKED data_INQUIRY_STANDARD_t {
	uint8_t peripheral;		// [7:5] Qualifier; [4:0] Device Type
	uint8_t peripheral_flags;	// [7] RMB
	uint8_t version;
	uint8_t response;		// [7:4] Obs, Obs, NORMACA, HISUP
					// [3:0] Data format
	uint8_t additional;		// Additional Length (N - 4)
	uint8_t flags[3];		// SCCS, ACC, TPGS[2], 3PC, Reserved[2], PROTECT
					// BQUE (obs), ENCSERV, VS, MULTIP, MCHNGR, Obs, Obs, ADDR16(a)
					// Obs, Obs, WBUS16(a), SYNC(a), LINKED (obs), Obs, CMDQUE, VS
	uint8_t vendor[8];		// T10 Vendor Identification
	uint8_t product[16];		// Product Identification
	uint8_t revision[4];		// Product Revision Level
	uint64_t serial;		// Drive Serial Number
	uint8_t unique[12];		// Vendor Unique
	uint8_t flag;			// [3:2] CLOCKING(a); [1:0] QAS(a), IUS(a)
	uint8_t RESERVED1;
	uint16_t version_descriptor[8];	// Version Descriptors
	uint8_t RESERVED2[22];
	uint8_t vendor_spec[0];
} data_INQUIRY_STANDARD_t;

typedef struct PACKED data_INQUIRY_VITAL_t {
	uint8_t peripheral;		// [7:5] Qualifier; [4:0] Device Type
	uint8_t page;			// Page Code
	uint16_t length;		// Page Length
	union {
		uint8_t payload[0];
	};
} data_INQUIRY_VITAL_t;

// READ CAPACITY (10)
typedef struct PACKED cmd_READ_CAPACITY_10_t {
	uint8_t op;			// 25h
	uint8_t RESERVED1;
	uint32_t lbaddr;		// Logical Block Address
	uint16_t RESERVED2;
	uint8_t pmi;			// [0] PMI
	uint8_t control;
} cmd_READ_CAPACITY_10_t;

typedef struct PACKED data_READ_CAPACITY_10_t {
	uint32_t lbaddr;		// Logical Block Address
	uint32_t lbsize;		// Block length in bytes
} data_READ_CAPACITY_10_t;

// READ (10)
typedef struct PACKED cmd_READ_10_t {
	uint8_t op;			// 28h
	uint8_t flags;			// [7:5] RDPROTECT; [4:0] DPO, FUA, Reserved, FUA_NV, Obsolete
	uint32_t lbaddr;		// Logical Block Address
	uint8_t group;			// Group Number
	uint16_t length;		// Transfer length
	uint8_t control;
} cmd_READ_10_t;

// WRITE (10)
typedef struct PACKED cmd_WRITE_10_t {
	uint8_t op;			// 2ah
	uint8_t flags;			// [7:5] WRPROTECT; [4:0] DPO, FUA, Reserved, FUA_NV, Obsolete
	uint32_t lbaddr;		// Logical Block Address
	uint8_t group;			// Group Number
	uint16_t length;		// Transfer length
	uint8_t control;
} cmd_WRITE_10_t;

// Command sizes
static const uint8_t cmd_size[256] = {
	[INQUIRY] = 6u,
	[REQUEST_SENSE] = 6u,
	[READ_CAPACITY_10] = 10u,
	[MODE_SENSE_6] = 6u,
	[READ_10] = 10u,
	[TEST_UNIT_READY] = 6u,
	[SYNCHRONIZE_CACHE_10] = 10u,
	[START_STOP_UNIT] = 6u,
	[WRITE_10] = 10u,
	[READ_FORMAT_CAPACITIES] = 10u,
	[SERVICE_ACTION_IN_16] = 16u,
	[PREVENT_ALLOW_MEDIUM_REMOVAL] = 6u,
	[ATA_PASS_THROUGH_16] = 16u,
};

#endif // SCSI_DEFS_CMDS_H
