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

	// Return type: status
	uint8_t (*sense)(scsi_t *scsi, uint8_t *sense,
		       uint8_t *asc, uint8_t *ascq);
	// Return != 0 for error
	uint32_t (*capacity)(scsi_t *scsi, uint32_t *lbnum, uint32_t *lbsize);

	// Read operations

	// Read: Start -> ((Available?) -> Data) -> Stop
	uint32_t (*read_start)(scsi_t *scsi, uint32_t offset, uint32_t size);
	// Return < 0 for error
	// Return unit: Bytes
	int32_t (*read_available)(scsi_t *scsi);
	// Returned address need to be aligned to 32-bytes boundary for DMA
	// Length unit: Bytes
	void *(*read_data)(scsi_t *scsi, uint32_t *length);
	uint32_t (*read_stop)(scsi_t *scsi);

	// Write operations

	// Write: Start -> (Data -> (Busy?)) -> Stop
	uint32_t (*write_start)(scsi_t *scsi, uint32_t offset, uint32_t size);
	// Length unit: Bytes
	uint32_t (*write_data)(scsi_t *scsi, uint32_t length, const void *p);
	// Return < 0 for error
	int32_t (*write_busy)(scsi_t *scsi);
	// Return != 0 for error
	uint32_t (*write_stop)(scsi_t *scsi);
} scsi_handlers_t;

// SCSI functions
scsi_t *scsi_init(const scsi_handlers_t *handlers);
scsi_ret_t scsi_cmd(scsi_t *scsi, const void *pdata, uint8_t size);
scsi_state_t scsi_data(scsi_t *scsi, const void *pdata, uint32_t size);
scsi_ret_t scsi_process(scsi_t *scsi, uint32_t maxsize);

#ifdef __cplusplus
}
#endif

#endif // SCSI_H
