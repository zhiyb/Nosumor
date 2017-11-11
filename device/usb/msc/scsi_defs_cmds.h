#ifndef SCSI_DEFS_CMDS_H
#define SCSI_DEFS_CMDS_H

#include <stdint.h>
#include <macros.h>

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
			uint8_t length;	// Additoinal CDB length
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

typedef struct PACKED cmd_INQUIRY_t {
	uint8_t op;
	uint8_t info;
	uint8_t page;
	uint16_t length;
	uint8_t control;
} cmd_INQUIRY_t;

#define CMD_INQUIRY_EVPD_Msk	0x01
#define CMD_INQUIRY_CMDDT_Msk	0x02

// Command sizes
static const uint8_t cmd_size[256] = {
	[INQUIRY] = 6u,
};

#endif // SCSI_DEFS_CMDS_H
