#ifndef SCSI_H
#define SCSI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct scsi_t scsi_t;

typedef enum {SCSIGood = 0, SCSIFailure, SCSIRead, SCSIWrite,
	      SCSIBusy = 0x80} scsi_state_t;

typedef struct scsi_ret_t {
	void *p;
	uint32_t length;
	scsi_state_t state;
} scsi_ret_t;

scsi_t *scsi_init();
scsi_ret_t scsi_cmd(scsi_t *scsi, const void *pdata, uint8_t size);
scsi_state_t scsi_data(scsi_t *scsi, const void *pdata, uint32_t size);
scsi_ret_t scsi_process(scsi_t *scsi, uint32_t maxsize);

/* External interface functions */

// Return type: status
extern uint8_t scsi_sense(scsi_t *scsi, uint8_t *sense,
			  uint8_t *asc, uint8_t *ascq);

// Return 0 for success
extern uint32_t scsi_capacity(scsi_t *scsi, uint32_t *lbnum, uint32_t *lbsize);

// Read: Start -> ((Available?) -> Data) -> Stop
extern uint32_t scsi_read_start(scsi_t *scsi, uint32_t offset, uint32_t size);
extern uint32_t scsi_read_available(scsi_t *scsi);
// Returned address need to be aligned to 32-bytes boundary for DMA transfer
extern void *scsi_read_data(scsi_t *scsi, uint32_t *length);
extern uint32_t scsi_read_stop(scsi_t *scsi);

// Write: Start -> (Data -> (Busy?)) -> Stop
extern uint32_t scsi_write_start(scsi_t *scsi, uint32_t offset, uint32_t size);
extern uint32_t scsi_write_data(scsi_t *scsi, uint32_t length, const void *p);
extern uint32_t scsi_write_busy(scsi_t *scsi);
extern uint32_t scsi_write_stop(scsi_t *scsi);

#ifdef __cplusplus
}
#endif

#endif // SCSI_H
