#ifndef VENDOR_DEFS_H
#define VENDOR_DEFS_H

#include <stdint.h>

#define PRODUCT_NAME	"Nosumor remake"

#define SW_VERSION	0x0004
#define SW_VERSION_STR	"0004"

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

typedef struct pong_t {
	uint16_t hw_ver;
	uint16_t sw_ver;
	uint32_t uid[3];
	uint16_t fsize;
} pong_t;

enum ReportINTypes {Pong = 0, FlashStatus};
enum ReportOUTTypes {Ping = 0, FlashReset, FlashData, FlashCheck, FlashStart,
		KeycodeUpdate, RGBUpdate};

#endif // VENDOR_DEFS_H
