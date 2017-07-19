#ifndef MMC_IO_H
#define MMC_IO_H

#include "../fatfs/integer.h"
#include "../fatfs/diskio.h"

DSTATUS mmc_disk_init();
DSTATUS mmc_disk_status();
DRESULT mmc_disk_read(BYTE *buff, DWORD sector, UINT count);

#endif // MMC_IO_H
