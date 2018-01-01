#ifndef SCSI_DEFS_CMDS_H
#define SCSI_DEFS_CMDS_H

#include <stdint.h>
#include <macros.h>
#include "scsi_defs_ops.h"

/* Command types */

typedef struct PACKED cmd6_t {	// 6-byte commands
	uint8_t op;
	union {
		uint8_t info;	// [7:5]
		uint8_t addr_h;	// [4:0]
	};
	uint16_t addr_l;
	uint8_t length;
	uint8_t control;
} cmd6_t;

typedef struct PACKED cmd10_t {	// 10-byte commands
	uint8_t op;
	union {
		uint8_t info1;	// [7:5]
		uint8_t action;	// [4:0]
	};
	uint32_t addr;
	uint8_t info2;
	uint16_t length;
	uint8_t control;
} cmd10_t;

typedef struct PACKED cmd12_t {	// 12-byte commands
	uint8_t op;
	union {
		uint8_t info1;	// [7:5]
		uint8_t action;	// [4:0]
	};
	uint32_t addr;
	uint32_t length;
	uint8_t info2;
	uint8_t control;
} cmd12_t;

typedef struct PACKED cmd16_t {	// 16-byte (long LBA) commands
	uint8_t op;
	union {
		uint8_t info1;	// [7:5] or [7:0] (long LBA)
		uint8_t action;	// [4:0]
	};
	union {
		struct {
			uint32_t addr;
			uint32_t data;
		};
		uint64_t addr64;
	};
	uint32_t length;
	uint8_t info2;
	uint8_t control;
} cmd16_t;

typedef struct PACKED cmdv_t {	// Variable length commands
	uint8_t op;		// 7Fh
	uint8_t control;
	uint8_t info[5];
	union {
		struct {
			uint8_t length;		// Additoinal CDB length
						// (n - 7), multiple of 4
			uint16_t action;
			uint8_t payload[0];
		};
		uint8_t raw[0];
	};
} cmdv_t;

typedef union PACKED cmd_t {
	uint8_t op;
	cmd6_t cmd6;
	cmd10_t cmd10;
	cmd12_t cmd12;
	cmd16_t cmd16;
	cmdv_t cmdv;
} cmd_t;

/* Additional command types */

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

// Command sizes
static const uint8_t cmd_size[256] = {
	[INQUIRY] = 6u,
	[REQUEST_SENSE] = 6u,
};

#endif // SCSI_DEFS_CMDS_H
