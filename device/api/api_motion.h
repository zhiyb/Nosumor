#ifndef API_MOTION_H
#define API_MOTION_H

#include "api_defs.h"

typedef struct PACKED api_motion_t {
	int32_t quat[4];
} api_motion_t;

extern const struct api_reg_t api_motion;

#endif // API_MOTION_H
