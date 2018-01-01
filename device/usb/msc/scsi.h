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

#ifdef __cplusplus
}
#endif

#endif // SCSI_H
