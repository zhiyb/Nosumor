#include <stdio.h>
#include <string.h>
#include <escape.h>
#include <peripheral/mpu.h>
#include <api/api_proc.h>
#include "api_motion.h"

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload);
const struct api_reg_t api_motion = {
	handler, 0x0002, "Motion"
};

static void handler(void *hid, uint8_t channel,
		    void *data, uint8_t size, void *payload)
{
	if (size != 0) {
		printf(ESC_ERROR "[MOTION] Invalid size: %u\n", size);
		return;
	}

	api_motion_t *p = (api_motion_t *)payload;
	memcpy(p->quat, (void *)mpu_quat(), sizeof(p->quat));
	memcpy(p->compass, (void *)mpu_compass(), sizeof(p->compass));
	api_send(hid, channel, sizeof(api_motion_t));
}
