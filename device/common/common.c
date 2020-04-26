#include <stdint.h>
#include <device.h>
#include <common.h>

static char toHex(char c)
{
	return (c < 10 ? '0' : 'a' - 10) + c;
}

void uid_str(char *s)
{
	// Construct serial number string from device UID
	char *c = s;
	uint8_t *p = (void *)UID_BASE + 11u;
	for (uint32_t i = 12; i != 0; i--) {
		*c++ = toHex(*p >> 4u);
		*c++ = toHex(*p-- & 0x0f);
	}
	*c = 0;
}
