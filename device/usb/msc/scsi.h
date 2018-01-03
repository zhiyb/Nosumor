#ifndef SCSI_H
#define SCSI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct scsi_t scsi_t;

typedef enum {Good = 0, Failure, Read, Write} scsi_state_t;

typedef struct scsi_ret_t {
	void *p;
	uint32_t length;
	scsi_state_t state;
} scsi_ret_t;

scsi_t *scsi_init();
scsi_ret_t scsi_cmd(scsi_t *scsi, const void *pdata, uint8_t size);
scsi_state_t scsi_data(scsi_t *scsi, const void *pdata, uint32_t size);

// Return 0 for success
extern uint32_t scsi_capacity(scsi_t *scsi, uint32_t *lbnum, uint32_t *lbsize);
// Returned address need to be aligned to 4-bytes boundary
extern void *scsi_read(scsi_t *scsi, uint32_t offset, uint32_t *length);
extern uint32_t scsi_write_start(scsi_t *scsi, uint32_t offset, uint32_t size);
extern uint32_t scsi_write_data(scsi_t *scsi, uint32_t length, const void *p);
extern uint32_t scsi_write_stop(scsi_t *scsi);

#ifdef __cplusplus
}
#endif

#endif // SCSI_H
