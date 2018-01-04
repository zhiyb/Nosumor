#ifndef MMC_IO_H
#define MMC_IO_H

#include <stdint.h>
#include "../fatfs/integer.h"
#include "../fatfs/diskio.h"
#include "../usb/msc/scsi.h"

// Unit: 512 bytes
uint32_t mmc_capacity();
// Blocks transferred
uint32_t mmc_statistics();

// FatFs interface functions
DSTATUS mmc_disk_init();
DSTATUS mmc_disk_status();
DRESULT mmc_disk_read(BYTE *buff, DWORD sector, UINT count);

// SCSI interface functions
uint32_t scsi_capacity(scsi_t *scsi, uint32_t *lbnum, uint32_t *lbsize);
void *scsi_read(scsi_t *scsi, uint32_t offset, uint32_t *length);
uint32_t scsi_write_start(scsi_t *scsi, uint32_t offset, uint32_t size);
uint32_t scsi_write_data(scsi_t *scsi, uint32_t length, const void *p);
uint32_t scsi_write_busy(scsi_t *scsi);
uint32_t scsi_write_stop(scsi_t *scsi);

#endif // MMC_IO_H
