#ifndef SCSI_DEFS_SENSE_H
#define SCSI_DEFS_SENSE_H

#include <stdint.h>
#include <macros.h>

// Response code
#define SENSE_CURRENT_FIXED	0x70
#define SENSE_DEFERRED_FIXED	0x71
#define SENSE_CURRENT_DESC	0x72
#define SENSE_DEFERRED_DESC	0x73
#define SENSE_VENDOR		0x7f

// Status Codes
// http://www.t10.org/lists/2status.htm
#define GOOD	0x00
#define CHECK_CONDITION	0x02
#define CONDITION_MET	0x04
#define BUSY	0x08
#define RESERVATION_CONFLICT	0x18
#define TASK_SET_FULL	0x28
#define ACA_ACTIVE	0x30
#define TASK_ABORTED	0x40

// Sense keys
// http://www.t10.org/lists/2sensekey.htm
#define NO_SENSE	0x00
#define RECOVERED_ERROR	0x01
#define NOT_READY	0x02
#define MEDIUM_ERROR	0x03
#define HARDWARE_ERROR	0x04
#define ILLEGAL_REQUEST	0x05
#define UNIT_ATTENTION	0x06
#define DATA_PROTECT	0x07
#define BLANK_CHECK	0x08
#define VENDOR_SPECIFIC	0x09
#define COPY_ABORTED	0x0a
#define ABORTED_COMMAND	0x0b
#define VOLUME_OVERFLOW	0x0d
#define MISCOMPARE	0x0e
#define COMPLETED	0x0f

// ASC/ASCQ Assignments
// http://www.t10.org/lists/asc-num.htm
// Not included

/* Sense data formats */

typedef struct PACKED sense_fixed_t {
	uint8_t response;	// [7]: Valid INFORMATION field
	uint8_t OBSOLETE;
	uint8_t key;		// [7:5]: Flags; Sense key
	uint32_t info;		// INFORMATION
	uint8_t length;		// Additional sense length (n - 7)
	uint32_t cmd_info;	// Command-specific information
	uint8_t sense;		// Additional sense code
	uint8_t qualifier;	// Additional sense code qualifier
	uint8_t unit;		// Field replaceable unit code
	uint8_t specific[3];	// [23]: SKSV; Sense key specific
	uint8_t payload[0];	// Additional sense bytes
} sense_fixed_t;

typedef struct PACKED sense_desc_t {
	uint8_t response;	// Response code
	uint8_t key;		// Sense key
	uint8_t sense;		// Additional sense code
	uint8_t qualifier;	// Additional sense code qualifier
	uint8_t RESERVED[3];
	uint8_t length;		// Additional sense length (n - 7)
	// Sense data descriptor(s)
	uint8_t desc[0];
} sense_desc_t;

#endif // SCSI_DEFS_SENSE_H
