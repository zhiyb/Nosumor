#include <stdint.h>
#include <debug.h>
#include <fatfs/ff.h>
#include <peripheral/mmc.h>
#include <usb/msc/scsi.h>
#include <usb/msc/scsi_defs_sense.h>
#include "flash_scsi.h"

#define LBSZ	512ul

// Memory regions
extern char __app_start__, __app_end__;
extern char __conf_start__, __conf_end__;

typedef enum {Good = 0, RangeError,
	      UnlockFailed, EraseError, ProgramError} status_t;

typedef struct flash_t {
	void * const start, * const end;
	const uint32_t sector, ssize;
	status_t status;
	void *read, *write;
	uint32_t length;
} flash_t;

static flash_t flash[2] = {
	{&__conf_start__, &__conf_end__, 1, 16ul * 1024ul, Good, 0, 0, 0},
	{&__app_start__, &__app_end__, 5, 128ul * 1024ul, Good, 0, 0, 0},
};

// Borrow buffer space from MMC
extern void *mmc_scsi_buffer(uint32_t *length);

SECTION(.iram) STATIC_INLINE void flash_wait()
{
	__DSB();
	while (FLASH->SR & FLASH_SR_BSY_Msk);
}

static status_t flash_unlock()
{
	if (FLASH->CR & FLASH_CR_LOCK_Msk) {
		FLASH->KEYR = 0x45670123;
		FLASH->KEYR = 0xcdef89ab;

		flash_wait();
		if (FLASH->CR & FLASH_CR_LOCK_Msk) {
			printf(ESC_ERROR "[FLASH] Unlock failed\n");
			dbgbkpt();
			return UnlockFailed;
		}
	}
	return Good;
}

static status_t flash_program(void *dst, const void *src, uint32_t size)
{
	if (size == 0)
		return Good;

#ifdef DEBUG
	// Check for word alignment
	if (((uint32_t)dst & 3u) || ((uint32_t)src & 3u) || (size & 3u)) {
		dbgbkpt();
		return RangeError;
	}
#endif

	printf(ESC_WRITE "[FLASH] Program: "
	       ESC_DATA "@%p <= @%p+%lu\n", dst, src, size);

	// Align to 4-byte boundary
	size >>= 2u;
	uint32_t *fp = dst;
	const uint32_t *bp = src;

	// Unlock flash
	status_t status = flash_unlock();
	if (status != Good)
		return status;

	// Program size x32
	FLASH->CR = (0b10 << FLASH_CR_PSIZE_Pos) | FLASH_CR_PG_Msk;

	// Program flash
	for (; size; size--, fp++, bp++) {
		if (*fp == *bp)
			continue;

		__disable_irq();
		*fp = *bp;
		flash_wait();
		__enable_irq();

		// Check for errors
		if (FLASH->SR & (FLASH_SR_ERSERR_Msk | FLASH_SR_WRPERR_Msk |
				 FLASH_SR_PGPERR_Msk | FLASH_SR_PGAERR_Msk)) {
			printf(ESC_ERROR "[FLASH] Program failed: "
			       ESC_DATA "0x%08lx @%p <= @%p+%lu\n",
			       FLASH->SR, fp, bp, size);
			dbgbkpt();
			// Clear flags
			FLASH->SR = FLASH->SR;
			return ProgramError;
		}
	}
	return Good;
}

static status_t flash_program_region(uint32_t idx, uint32_t offset,
				     const void *src, uint32_t size)
{
	return flash_program(flash[idx].start + offset, src, size);
}

static status_t flash_erase_sector(uint32_t sec)
{
	printf(ESC_WRITE "[FLASH] Erase sector " ESC_DATA "%lu\n", sec);

	// Unlock flash
	status_t status = flash_unlock();
	if (status != Good)
		return status;

	// Erase flash sector
	// Program size x32
	FLASH->CR = (0b10 << FLASH_CR_PSIZE_Pos) | FLASH_CR_SER_Msk |
			(sec << FLASH_CR_SNB_Pos);
	// Start sector erase
	__disable_irq();
	FLASH->CR |= FLASH_CR_STRT_Msk;
	// Wait for completion
	flash_wait();
	__enable_irq();

	// Check for erase errors
	if (FLASH->SR & (FLASH_SR_ERSERR_Msk | FLASH_SR_WRPERR_Msk |
			 FLASH_SR_PGPERR_Msk | FLASH_SR_PGAERR_Msk)) {
		printf(ESC_ERROR "[FLASH] Erase failed: "
		       ESC_DATA "0x%08lx @%lu\n", FLASH->SR, sec);
		dbgbkpt();
		// Clear flags
		FLASH->SR = FLASH->SR;
		return EraseError;
	}
	return Good;
}

static status_t flash_erase_region(uint32_t idx, uint32_t offset, uint32_t size)
{
	if (size == 0ul)
		return Good;

#ifdef DEBUG
	// Check for word alignment
	if ((offset & 7ul) || (size & 7ul)) {
		dbgbkpt();
		return RangeError;
	}
#endif

	// Check for sector boundaries
	uint32_t ssize = flash[idx].ssize;
	// First sector
	uint32_t fs = offset / ssize;
	// Last sector
	uint32_t ls = (offset + size - 1ul) / ssize;
	// Sector boundary crossed
	if (ls != fs) {
		// First sector
		uint32_t s = ssize - (offset % ssize);
		status_t status = flash_erase_region(idx, offset, s);
		if (status != Good)
			return status;
		offset += s;
		size -= s;
		// Complete sectors
		while (++fs != ls) {
			status = flash_erase_region(idx, offset, ssize);
			if (status != Good)
				return status;
			offset += ssize;
			size -= ssize;
		}
		// Last sector
		status = flash_erase_region(idx, offset, size);
		return status;
	}

	dbgprintf(ESC_WRITE "[FLASH %lu] Erase: "
		  ESC_DATA "%lu+%lu\n", idx, offset, size);

	// Offset to the start of the sector
	offset = (offset % ssize) >> 3ul;
	// Sector start address
	uint64_t *start = flash[idx].start + fs * ssize;
	// Erased pattern
	static const uint64_t full = 0xffffffffffffffff;
	// Borrow buffer space
	uint64_t *buf = mmc_scsi_buffer(0);
	// 8-byte aligned
	size >>= 3ul;
	ssize >>= 3ul;

	// Check if erase actually needed
	uint64_t *fp = start + offset;
	uint64_t tmp = full;
	for (uint32_t i = size; i; i--) {
		tmp &= *fp++;
		if (tmp != full)
			break;
	}
	// All 1s, no erase needed
	if (tmp == full) {
		dbgprintf(ESC_DEBUG "[FLASH %lu] Erase not necessary\n", idx);
		return Good;
	}

	// Copy valid data at the front
	uint64_t *bp = buf;
	fp = start;
	tmp = full;
	for (uint32_t i = offset; i; i--) {
		uint64_t v = *fp++;
		*bp++ = v;
		tmp &= v;
	}

	// Copy valid data at the back
	bp = buf + ssize;
	fp = start + ssize;
	for (uint32_t i = ssize - offset - size; i; i--) {
		uint64_t v = *--fp;
		*--bp = v;
		tmp &= v;
	}

	// Data saved, erase entire sector
	status_t status = flash_erase_sector(flash[idx].sector + fs);
	if (status != Good)
		return status;

	// If no recovery needed, finish
	if (tmp == full)
		return status;

	// Program data at the front
	status = flash_program(start, buf, offset << 3u);
	if (status != Good)
		return status;

	// Program data at the back
	offset += size;
	status = flash_program(start + offset, buf + offset,
			       (ssize - offset) << 3u);

	dbgprintf(ESC_WRITE "[FLASH %lu] Sector "
		  ESC_DATA "%lu" ESC_WRITE " recovered\n", idx, fs);
	return status;
}

static status_t flash_erase_all(uint32_t idx)
{
	return flash_erase_region(idx, 0, flash[idx].end - flash[idx].start);
}

/* SCSI interface functions */

static uint8_t scsi_sense(void *p, uint8_t *sense, uint8_t *asc, uint8_t *ascq)
{
	uint32_t idx = (uint32_t)p;
	status_t status = flash[idx].status;
	flash[idx].status = Good;

	switch (status) {
	case Good:
		*sense = NO_SENSE;
		// 00/00  DZTPROMAEBKVF  NO ADDITIONAL SENSE INFORMATION
		*asc = 0x00;
		*ascq = 0x00;
		return GOOD;
	case RangeError:
		*sense = ILLEGAL_REQUEST;
		// 21/00  DZT RO   BK    LOGICAL BLOCK ADDRESS OUT OF RANGE
		*asc = 0x21;
		*ascq = 0x00;
		return CHECK_CONDITION;
	case UnlockFailed:
		*sense = DATA_PROTECT;
		// 27/00  DZT RO   BK    WRITE PROTECTED
		*asc = 0x27;
		*ascq = 0x00;
		return CHECK_CONDITION;
	case EraseError:
		*sense = MEDIUM_ERROR;
		// 51/00  DZT RO         ERASE FAILURE
		*asc = 0x51;
		*ascq = 0x00;
		return CHECK_CONDITION;
	case ProgramError:
		*sense = MEDIUM_ERROR;
		// 0C/00  DZT R          WRITE ERROR
		*asc = 0x0c;
		*ascq = 0x00;
		return CHECK_CONDITION;
	default:
		*sense = HARDWARE_ERROR;
		// 04/00  DZTPROMAEBKVF  LOGICAL UNIT NOT READY, CAUSE NOT REPORTABLE
		*asc = 0x04;
		*ascq = 0x00;
		return CHECK_CONDITION;
	}
}

static uint32_t scsi_capacity(void *p, uint32_t *lbnum, uint32_t *lbsize)
{
	uint32_t idx = (uint32_t)p;
	*lbsize = LBSZ;
	*lbnum = (flash[idx].end - flash[idx].start) / LBSZ;
	return 0;
}

/* Read operations */

static uint32_t scsi_read_start(void *p, uint32_t offset, uint32_t size)
{
	uint32_t idx = (uint32_t)p;
	if (size == 0)
		return 0;
	else if (offset + size > (flash[idx].end - flash[idx].start) / LBSZ) {
		flash[idx].status = RangeError;
		printf(ESC_ERROR "[FLASH %lu] LBA out of range: "
		       ESC_DATA "%lu+%lu\n", idx, offset, size);
		dbgbkpt();
		return 0;
	}

	flash[idx].read = flash[idx].start + offset * LBSZ;
	flash[idx].length = size * LBSZ;
	return size;
}

static int32_t scsi_read_available(void *p)
{
	uint32_t idx = (uint32_t)p;
	return flash[idx].length;
}

static void *scsi_read_data(void *p, uint32_t *length)
{
	uint32_t idx = (uint32_t)p;
	void *ptr = flash[idx].read;
	flash[idx].read += *length;
	flash[idx].length -= *length;
	return ptr;
}

static uint32_t scsi_read_stop(void *p)
{
	uint32_t idx = (uint32_t)p;
	flash[idx].length = 0;
	return 0;
}

/* Write operations */
static uint32_t scsi_write_start(void *p, uint32_t offset, uint32_t size)
{
	uint32_t idx = (uint32_t)p;
	if (size == 0)
		return 0;
	else if (offset + size >
		 (flash[idx].end - flash[idx].start) / LBSZ) {
		flash[idx].status = RangeError;
		printf(ESC_ERROR "[FLASH %lu] LBA out of range: "
		       ESC_DATA "%lu+%lu\n", idx, offset, size);
		dbgbkpt();
		return 0;
	}
	dbgprintf(ESC_WRITE "[FLASH %lu] Write: "
		  ESC_DATA "%lu+%lu\n", idx, offset, size);

	// Erase related sectors
	flash[idx].status = flash_erase_region(idx, offset * LBSZ, size * LBSZ);
	if (flash[idx].status != Good)
		return 0;

	// Set up write operation
	flash[idx].write = flash[idx].start + offset * LBSZ;
	flash[idx].length = size * LBSZ;
	return size;
}

static uint32_t scsi_write_data(void *p, uint32_t length, const void *data)
{
	uint32_t idx = (uint32_t )p;
	flash[idx].status = flash_program(flash[idx].write, data, length);
	if (flash[idx].status != Good)
		return 0;

	// Update status
	flash[idx].write += length;
	flash[idx].length -= length;
	return length;
}

static int32_t scsi_write_busy(void *p)
{
	return FLASH->SR & FLASH_SR_BSY_Msk;
}

static uint32_t scsi_write_stop(void *p)
{
	uint32_t idx = (uint32_t)p;

	// Clear flash operations
	FLASH->CR = 0;
	flash_wait();

	// Check for errors
	if (FLASH->SR & (FLASH_SR_ERSERR_Msk | FLASH_SR_WRPERR_Msk |
			 FLASH_SR_PGPERR_Msk | FLASH_SR_PGAERR_Msk)) {
		flash[idx].status = ProgramError;
		printf(ESC_ERROR "[FLASH %lu] Program failed: 0x%08lx",
		       idx, FLASH->SR);
		dbgbkpt();
		// Clear flags
		FLASH->SR = FLASH->SR;
		return -1;
	}
	return 0;
}

static const char *scsi_name(void *p)
{
	uint32_t region = (uint32_t)p;
	static const char *names[] = {
		"Configure",
		"Firmware",
	};
	return names[region];
}

scsi_if_t flash_scsi_handlers(uint32_t region)
{
	static const scsi_handlers_t handlers = {
		SCSIRemovable,
		scsi_name,
		scsi_sense,
		scsi_capacity,

		scsi_read_start,
		scsi_read_available,
		scsi_read_data,
		scsi_read_stop,

		scsi_write_start,
		scsi_write_data,
		scsi_write_busy,
		scsi_write_stop,
	};
	return (scsi_if_t){&handlers, (void *)region};
}

/* FatFs interface functions */

DSTATUS flash_disk_status(uint32_t idx)
{
	return 0;
}

DSTATUS flash_disk_init(uint32_t idx)
{
	return RES_OK;
}

DRESULT flash_disk_read(uint32_t idx, BYTE *buff, DWORD sector, UINT count)
{
	memcpy(buff, flash[idx].start + sector * 512u, count * 512u);
	return RES_OK;
}

DRESULT flash_disk_write(uint32_t idx, const BYTE *buff,
			 DWORD sector, UINT count)
{
	sector *= 512ul;
	count *= 512ul;
	if (flash_erase_region(idx, sector, count))
		return RES_WRPRT;
	if (flash_program_region(idx, sector, buff, count))
		return RES_ERROR;
	return RES_OK;
}

DRESULT flash_disk_ioctl(uint32_t idx, BYTE cmd, void *buff)
{
	DWORD dword;
	switch (cmd) {
	case GET_BLOCK_SIZE:
		dword = 1;//LBSZ;
		memcpy(buff, &dword, sizeof(DWORD));
		return RES_OK;
	case GET_SECTOR_COUNT:
		dword = (flash[idx].end - flash[idx].start) / 512ul;
		memcpy(buff, &dword, sizeof(DWORD));
		return RES_OK;
	case CTRL_SYNC:
		return RES_OK;
	default:
		dbgbkpt();
		return RES_ERROR;
	}
}

uint32_t flash_fatfs_init(uint32_t idx, uint32_t erase)
{
	static const char *vols[] = {"CONF:", "APP:"};
	static const char *labels[] = {"CONF:Configure", "APP:Firmware"};
	static FATFS fs SECTION(.dtcm);
	FRESULT res;
	memset(&fs, 0, sizeof(fs));

	if (!erase) {
		// Mount volume
		if ((res = f_mount(&fs, vols[idx], 1)) != FR_OK) {
			dbgprintf(ESC_ERROR "[FLASH %lu] Mount failed: "
				  ESC_DATA "%u\n", idx, res);
			goto erase;
		}
		printf(ESC_GOOD "[FLASH %lu] Mount OK, skip\n", idx);

		// Unmount
		if ((res = f_unmount(vols[idx])) != FR_OK)
			printf(ESC_WARNING "[FLASH %lu] Unmount failed: "
			       ESC_DATA "%u\n", idx, res);

		return FR_OK;
	}

	// Erase entire flash as initialisation
erase:	flash_erase_all(idx);

	// Recreate volume
	uint8_t buf[1024];
	if ((res = f_mkfs(vols[idx], FM_FAT | FM_SFD, 512,
			  buf, sizeof(buf))) != FR_OK) {
		printf(ESC_ERROR "[FLASH %lu] Create volume failed: "
		       ESC_DATA "%u\n", idx, res);
		return res;
	}

	// Mount volume
	if ((res = f_mount(&fs, vols[idx], 1)) != FR_OK) {
		printf(ESC_ERROR "[FLASH %lu] Mount failed: "
			  ESC_DATA "%u\n", idx, res);
		return res;
	}

	// Set label
	if ((res = f_setlabel(labels[idx])) != FR_OK)
		printf(ESC_WARNING "[FLASH %lu] Set volume label failed: "
		       ESC_DATA "%u\n", idx, res);

	// Unmount
	if ((res = f_unmount(vols[idx])) != FR_OK)
		printf(ESC_WARNING "[FLASH %lu] Unmount failed: "
		       ESC_DATA "%u\n", idx, res);

	return FR_OK;
}
