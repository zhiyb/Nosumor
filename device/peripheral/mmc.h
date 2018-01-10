#ifndef MMC_IO_H
#define MMC_IO_H

#include <stdint.h>
#include <fatfs/integer.h>
#include <fatfs/diskio.h>
#include <usb/msc/scsi.h>

// Unit: 512 bytes
uint32_t mmc_capacity();
// Blocks transferred
uint32_t mmc_statistics();

// FatFs interface functions
DSTATUS mmc_disk_init();
DSTATUS mmc_disk_status();
DRESULT mmc_disk_read(BYTE *buff, DWORD sector, UINT count);

/* SCSI interface function handlers */
scsi_if_t mmc_scsi_handlers();

#endif // MMC_IO_H
