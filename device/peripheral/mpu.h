#ifndef MPU_H
#define MPU_H

// Axes orientation
//             |
//        +----Z--------+
//       /     |       /|
//      *-------------* |
//      |             | |
//      |  **     **  | |
// --X--|  **  O  **  | |
//      |     /       | |
//      |    / * * *  |-+
//      |   Y         |/
//      *--/----------*

#include <stdint.h>
#include <usb/hid/usb_hid.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MPU_I2C_ADDR	0b1101000
#define AK8963_I2C_ADDR	0x0c

uint32_t mpu_sys_init(void *i2c);
void mpu_usb_hid(usb_hid_if_t *hid, usb_hid_if_t *hid_mouse);
volatile int16_t *mpu_accel();
volatile int32_t *mpu_gyro();
volatile int16_t *mpu_accel_avg();
volatile int16_t *mpu_gyro_avg();
volatile int32_t *mpu_quat();
volatile int16_t *mpu_compass();
uint32_t mpu_cnt();

#ifdef __cplusplus
}
#endif

#endif // MPU_H
