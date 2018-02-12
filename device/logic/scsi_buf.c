#include <stdint.h>
#include <macros.h>

#ifdef BOOTLOADER
#define BUFFER_SIZE	(128 * 1024)
#else
#define BUFFER_SIZE	(16 * 1024)
#endif

static uint8_t scsi_buf[BUFFER_SIZE] ALIGN(32);

void *scsi_buffer(uint32_t *length)
{
	if (length)
		*length = sizeof(scsi_buf);
	return scsi_buf;
}
