#ifndef API_CONFIG_PRIV_H
#define API_CONFIG_PRIV_H

#include <stdint.h>

extern struct api_config_data_t {
	uint8_t keyboard, mouse, joystick, microSD, flash;
} api_config_data;

#endif // API_CONFIG_PRIV_H
