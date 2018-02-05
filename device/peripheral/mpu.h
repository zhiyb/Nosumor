#ifndef MPU_H
#define MPU_H

#include <stdint.h>

#define MPU_I2C_ADDR	0b1101000
#define AK8963_I2C_ADDR	0x0c

uint32_t mpu_init(void *i2c);
void mpu_process();
volatile int16_t *mpu_accel();
volatile int16_t *mpu_gyro();

#endif // MPU_H
