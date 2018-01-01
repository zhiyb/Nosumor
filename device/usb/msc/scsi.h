#ifndef SCSI_H
#define SCSI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct scsi_t scsi_t;

typedef struct scsi_ret_t {
	void *p;
	uint32_t length;
	uint8_t failure;
} scsi_ret_t;

scsi_t *scsi_init();
scsi_ret_t scsi_cmd(scsi_t *scsi, const void *pdata, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif // SCSI_H
