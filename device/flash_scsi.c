#include <stdint.h>
#include <peripheral/mmc.h>
#include <usb/msc/scsi.h>
#include <usb/msc/scsi_defs_sense.h>
#include <debug.h>

#define F	0

// Memory regions
extern char __app_start__, __app_end__;
extern char __conf_start__, __conf_end__;

typedef enum {Good = 0, RangeError,
	      UnlockFailed, EraseError, ProgramError} state_t;

typedef struct flash_t {
	void * const start, * const end;
	const uint32_t sector, bsize;
	state_t state;
	void *read, *write;
	uint32_t length;
} flash_t;

static flash_t flash[2] = {
	{&__conf_start__, &__conf_end__, 1, 16ul * 1024ul, Good, 0, 0, 0},
	{&__app_start__, &__app_end__, 5, 128ul * 1024ul, Good, 0, 0, 0},
};

SECTION(.iram) STATIC_INLINE void flash_wait()
{
	__DSB();
	while (FLASH->SR & FLASH_SR_BSY_Msk);
}

/* SCSI interface functions */

static uint8_t scsi_sense(scsi_t *scsi, uint8_t *sense, uint8_t *asc, uint8_t *ascq)
{
	state_t state = flash[F].state;
	flash[F].state = Good;

	switch (state) {
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

static uint32_t scsi_capacity(scsi_t *scsi, uint32_t *lbnum, uint32_t *lbsize)
{
	*lbsize = flash[F].bsize;
	*lbnum = (flash[F].end - flash[F].start) / flash[F].bsize;
	return 0;
}

/* Read operations */

static uint32_t scsi_read_start(scsi_t *scsi, uint32_t offset, uint32_t size)
{
	if (size == 0)
		return 0;
	else if (offset + size >
		 (flash[F].end - flash[F].start) / flash[F].bsize) {
		flash[F].state = RangeError;
		return 0;
	}

	flash[F].read = flash[F].start + offset * flash[F].bsize;
	flash[F].length = size * flash[F].bsize;
	return size;
}

static int32_t scsi_read_available(scsi_t *scsi)
{
	return flash[F].length;
}

static void *scsi_read_data(scsi_t *scsi, uint32_t *length)
{
	void *p = flash[F].read;
	flash[F].read += *length;
	flash[F].length -= *length;
	return p;
}

static uint32_t scsi_read_stop(scsi_t *scsi)
{
	flash[F].length = 0;
	return 0;
}

/* Write operations */

static uint32_t scsi_write_start(scsi_t *scsi, uint32_t offset, uint32_t size)
{
	if (size == 0)
		return 0;
	else if (offset + size >
		 (flash[F].end - flash[F].start) / flash[F].bsize) {
		flash[F].state = RangeError;
		return 0;
	}

	// Clear flags
	FLASH->SR = FLASH_SR_ERSERR_Msk | FLASH_SR_WRPERR_Msk |
			FLASH_SR_PGPERR_Msk | FLASH_SR_PGAERR_Msk;
	flash_wait();

	// Unlock flash
	if (FLASH->CR & FLASH_CR_LOCK_Msk) {
		FLASH->KEYR = 0x45670123;
		FLASH->KEYR = 0xcdef89ab;

		flash_wait();
		if (FLASH->CR & FLASH_CR_LOCK_Msk) {
			flash[F].state = UnlockFailed;
			return 0;
		}
	}

	// Erase flash sectors
	uint32_t sec = flash[F].sector + offset;
	for (uint32_t i = size; i != 0; i--) {
		// Program size x32
		FLASH->CR = (0b10 << FLASH_CR_PSIZE_Pos) | FLASH_CR_SER_Msk |
				(sec++ << FLASH_CR_SNB_Pos);
		// Start sector erase
		FLASH->CR |= FLASH_CR_STRT_Msk;
		// Wait for completion
		flash_wait();
	}

	// Check for erase errors
	if (FLASH->SR & (FLASH_SR_ERSERR_Msk | FLASH_SR_WRPERR_Msk |
			 FLASH_SR_PGPERR_Msk | FLASH_SR_PGAERR_Msk)) {
		flash[F].state = EraseError;
		return 0;
	}

	// Set up write operation
	// Program size x32
	FLASH->CR = (0b00 << FLASH_CR_PSIZE_Pos) | FLASH_CR_PG_Msk;
	flash[F].write = flash[F].start + offset * flash[F].bsize;
	flash[F].length = size * flash[F].bsize;
	return size;
}

static uint32_t scsi_write_data(scsi_t *scsi, uint32_t length, const void *p)
{
	// Align to 32-bit boundary
	uint32_t *ptr = flash[F].write;
	const uint32_t *dptr = p;
	uint32_t size = length >> 2u;

	// Program flash
	while (size--) {
		*ptr++ = *dptr++;
		flash_wait();

		// Check for program errors
		if (FLASH->SR & (FLASH_SR_ERSERR_Msk | FLASH_SR_WRPERR_Msk |
				 FLASH_SR_PGPERR_Msk | FLASH_SR_PGAERR_Msk)) {
			flash[F].state = ProgramError;
			return 0;
		}
	}

	flash[F].length -= length;
	return length;
}

static int32_t scsi_write_busy(scsi_t *scsi)
{
	return FLASH->SR & FLASH_SR_BSY_Msk;
}

static uint32_t scsi_write_stop(scsi_t *scsi)
{
	// Clear flash operations
	FLASH->CR = 0;
	flash_wait();

	// Check for errors
	if (FLASH->SR & (FLASH_SR_ERSERR_Msk | FLASH_SR_WRPERR_Msk |
			 FLASH_SR_PGPERR_Msk | FLASH_SR_PGAERR_Msk)) {
		flash[F].state = ProgramError;
		return -1;
	}
	return 0;
}

static const char *scsi_name()
{
	return "System Flash";
}

const scsi_handlers_t *flash_scsi_handlers()
{
	static const scsi_handlers_t handlers = {
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
	return &handlers;
}
