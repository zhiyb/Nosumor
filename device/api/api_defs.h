#ifndef API_DEFS_H
#define API_DEFS_H

#include <stdint.h>

#define MANUFACTURER	"zhiyb"

#define PRODUCT		"Nosumor"
#define PRODUCT_NAME	"Nosumor remake"

#define SW_VERSION	0x0004
#define SW_VERSION_STR	"0004"

#define API_BASE_SIZE	(4u)
#define API_DATA_SIZE	(32u)
#define API_REPORT_SIZE	(API_BASE_SIZE + API_DATA_SIZE)

#ifndef PACKED
#define PACKED	__attribute__((packed))
#endif

typedef union api_report_t {
	struct PACKED {
		uint8_t id;
		uint8_t size;
		uint8_t cksum;
		uint8_t channel;
		uint8_t payload[];
	};
	uint8_t raw[API_REPORT_SIZE];
} api_report_t;

typedef struct PACKED api_info_t {
	uint8_t channel;
	uint16_t version;
	char name[];
} api_info_t;

enum Positions {Top = 0x10, Bottom = 0x20, Left = 0x01, Right = 0x02};

enum APIChannels {InfoChannel = 0};

#endif // API_DEFS_H
