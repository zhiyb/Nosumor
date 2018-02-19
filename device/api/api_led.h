#ifndef API_LED_H
#define API_LED_H

#include "api_defs.h"

typedef struct PACKED api_led_info_t {
	uint8_t num;
	struct PACKED {
		uint8_t position;
		uint8_t type;
	} led[];
} api_led_info_t;

typedef struct PACKED api_led_config_t {
	uint8_t id;
	uint16_t clr[];
} api_led_config_t;

extern const struct api_reg_t api_led;

#endif // API_PING_H
