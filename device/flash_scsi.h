#ifndef FLASH_SCSI_H
#define FLASH_SCSI_H

typedef struct scsi_handlers_t scsi_handlers_t;

/* SCSI interface function handlers */
const scsi_handlers_t *flash_scsi_handlers();

#endif // FLASH_SCSI_H
