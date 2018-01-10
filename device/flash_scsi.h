#ifndef FLASH_SCSI_H
#define FLASH_SCSI_H

#include <stdint.h>
#include <usb/msc/scsi.h>

// Available flash regions
#define FLASH_CONF	0
#define FLASH_APP	1

/* SCSI interface function handlers */
scsi_if_t flash_scsi_handlers(uint32_t region);

#endif // FLASH_SCSI_H
