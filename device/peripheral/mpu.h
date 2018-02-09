#ifndef MPU_H
#define MPU_H

#include <stdint.h>
#include <usb/hid/usb_hid.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MPU_I2C_ADDR	0b1101000
#define AK8963_I2C_ADDR	0x0c

uint32_t mpu_init(void *i2c);
void mpu_usb_hid(usb_hid_if_t *hid);
volatile int16_t *mpu_accel();
volatile int16_t *mpu_gyro();

#ifdef __cplusplus
}
#endif

#endif // MPU_H
