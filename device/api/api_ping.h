#ifndef API_PING_H
#define API_PING_H

#include "api_defs.h"

typedef struct PACKED api_pong_t {
	uint16_t hw_ver;
	uint16_t sw_ver;
	uint32_t uid[3];
	uint16_t fsize;
} api_pong_t;

extern const struct api_reg_t api_ping;

#endif // API_PING_H
