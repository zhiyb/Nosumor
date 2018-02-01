#ifndef FLASH_SCSI_H
#define FLASH_SCSI_H

#include <stdint.h>
#include <fatfs/integer.h>
#include <fatfs/diskio.h>
#include <logic/scsi.h>

// Available flash regions
#define FLASH_CONF	0
#define FLASH_APP	1

uint32_t flash_fatfs_init(uint32_t idx, uint32_t erase);
uint32_t flash_stat_write(uint32_t idx);
uint32_t flash_stat_read(uint32_t idx);
uint32_t flash_busy();

// FatFs interface functions
DSTATUS flash_disk_status(uint32_t idx);
DSTATUS flash_disk_init(uint32_t idx);
DRESULT flash_disk_read(uint32_t idx, BYTE *buff, DWORD sector, UINT count);
DRESULT flash_disk_write(uint32_t idx, const BYTE *buff, DWORD sector, UINT count);
DRESULT flash_disk_ioctl(uint32_t idx, BYTE cmd, void *buff);

// SCSI interface function handlers
scsi_if_t flash_scsi_handlers(uint32_t region);

#endif // FLASH_SCSI_H
