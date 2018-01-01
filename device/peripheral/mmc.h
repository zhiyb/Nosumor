#ifndef MMC_IO_H
#define MMC_IO_H

#include <stdint.h>
#include "../fatfs/integer.h"
#include "../fatfs/diskio.h"

DSTATUS mmc_disk_init();
DSTATUS mmc_disk_status();
DRESULT mmc_disk_read(BYTE *buff, DWORD sector, UINT count);

// Unit: 512 bytes
uint32_t mmc_capacity();

#endif // MMC_IO_H
