#ifndef VENDOR_DEFS_H
#define VENDOR_DEFS_H

#include <stdint.h>

#define VENDOR_REPORT_BASE_SIZE	(3u)
#define VENDOR_REPORT_SIZE	(VENDOR_REPORT_BASE_SIZE + 32u)

typedef union vendor_report_t {
	struct {
		uint8_t id;
		uint8_t size;
		uint8_t type;
		uint8_t payload[];
	};
	uint8_t raw[VENDOR_REPORT_SIZE];
} vendor_report_t;

enum ReportINTypes {Pong = 0, FlashStatus};
enum ReportOUTTypes {Ping = 0, FlashReset, FlashData, FlashCheck, FlashStart};

#endif // VENDOR_DEFS_H
