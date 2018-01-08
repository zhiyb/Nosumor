#ifndef VENDOR_DEFS_H
#define VENDOR_DEFS_H

#include <stdint.h>

#define PRODUCT_NAME	"Nosumor remake"

#define SW_VERSION	0x0004
#define SW_VERSION_STR	"0004"

#define VENDOR_REPORT_BASE_SIZE	(3u)
#define VENDOR_REPORT_SIZE	(VENDOR_REPORT_BASE_SIZE + 32u)

#ifndef PACKED
#define PACKED	__attribute__((packed))
#endif

typedef union vendor_report_t {
	struct PACKED {
		uint8_t id;
		uint8_t size;
		uint8_t type;
		uint8_t payload[];
	};
	uint8_t raw[VENDOR_REPORT_SIZE];
} vendor_report_t;

typedef struct PACKED pong_t {
	uint16_t hw_ver;
	uint16_t sw_ver;
	uint32_t uid[3];
	uint16_t fsize;
} pong_t;

typedef struct PACKED led_info_t {
	uint8_t num;
	struct PACKED {
		uint8_t position;
		uint8_t type;
	} led[];
} led_info_t;

typedef struct PACKED led_config_t {
	uint8_t id;
	uint16_t clr[];
} led_config_t;

enum Positions {Top = 0x10, Bottom = 0x20, Left = 0x01, Right = 0x02};

enum ReportTypes {Ping = 0, FlashReset, FlashData, FlashCheck, FlashStart,
		KeycodeUpdate, LEDInfo, LEDConfig, Reply = 0x80};

#endif // VENDOR_DEFS_H
