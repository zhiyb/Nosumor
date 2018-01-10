#ifndef SCSI_H
#define SCSI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// SCSI object
typedef struct scsi_t scsi_t;

// SCSI return
typedef enum {SCSIGood = 0, SCSIFailure, SCSIRead, SCSIWrite,
	      SCSIBusy = 0x80} scsi_state_t;

// SCSI data return
typedef struct scsi_ret_t {
	void *p;
	uint32_t length;
	scsi_state_t state;
} scsi_ret_t;

// Interface handler functions
typedef struct scsi_handlers_t {
	// Generic

	const char *(*name)(void *data);
	// Return type: status
	uint8_t (*sense)(void *data, uint8_t *sense,
			 uint8_t *asc, uint8_t *ascq);
	// Return != 0 for error
	uint32_t (*capacity)(void *data, uint32_t *lbnum, uint32_t *lbsize);

	// Read operations

	// Read: Start -> ((Available?) -> Data) -> Stop
	uint32_t (*read_start)(void *data, uint32_t offset, uint32_t size);
	// Return < 0 for error
	// Return unit: Bytes
	int32_t (*read_available)(void *data);
	// Returned address need to be aligned to 32-bytes boundary for DMA
	// Length unit: Bytes
	void *(*read_data)(void *data, uint32_t *length);
	uint32_t (*read_stop)(void *data);

	// Write operations

	// Write: Start -> (Data -> (Busy?)) -> Stop
	uint32_t (*write_start)(void *data, uint32_t offset, uint32_t size);
	// Length unit: Bytes
	uint32_t (*write_data)(void *data, uint32_t length, const void *p);
	// Return < 0 for error
	int32_t (*write_busy)(void *data);
	// Return != 0 for error
	uint32_t (*write_stop)(void *data);
} scsi_handlers_t;

// SCSI interface register
typedef struct scsi_if_t {
	const scsi_handlers_t * const handlers;
	void *data;
} scsi_if_t;

// SCSI functions
scsi_t *scsi_init(const scsi_if_t iface);
scsi_ret_t scsi_cmd(scsi_t *scsi, const void *pdata, uint8_t size);
scsi_state_t scsi_data(scsi_t *scsi, const void *pdata, uint32_t size);
scsi_ret_t scsi_process(scsi_t *scsi, uint32_t maxsize);

#ifdef __cplusplus
}
#endif

#endif // SCSI_H
