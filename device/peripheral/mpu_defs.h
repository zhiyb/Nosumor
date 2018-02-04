#ifndef MPU_DEFS_H
#define MPU_DEFS_H

/* Registers
 * https://www.invensense.com/products/motion-tracking/9-axis/mpu-9250/
 */

// Gyroscope Self-Test Registers
#define SELF_TEST_X_GYRO	0x00
#define SELF_TEST_Y_GYRO	0x01
#define SELF_TEST_Z_GYRO	0x02

// Accelerometer Self-Test Registers
#define SELF_TEST_X_ACCEL	0x0D
#define SELF_TEST_Y_ACCEL	0x0E
#define SELF_TEST_Z_ACCEL	0x0F

// Gyro Offset Registers
#define XG_OFFSET_H	0x13
#define XG_OFFSET_L	0x14
#define YG_OFFSET_H	0x15
#define YG_OFFSET_L	0x16
#define ZG_OFFSET_H	0x17
#define ZG_OFFSET_L	0x18

// Sample Rate Divider
#define SMPLRT_DIV	0x19
// Configuration
#define CONFIG	0x1A
// Gyroscope Configuration
#define GYRO_CONFIG	0x1B
// Accelerometer Configuration
#define ACCEL_CONFIG	0x1C
// Accelerometer Configuration 2
#define ACCEL_CONFIG_2	0x1D
// Low Power Accelerometer ODR Control
#define LP_ACCEL_ODR	0x1E
// Wake-on Motion Threshold
#define WOM_THR	0x1F
// FIFO Enable
#define FIFO_EN	0x23

// I2C Master Control
#define I2C_MST_CTRL	0x24
// I2C Slave 0 Control
#define I2C_SLV0_ADDR	0x25
#define I2C_SLV0_REG	0x26
#define I2C_SLV0_CTRL	0x27
// I2C Slave 1 Control
#define I2C_SLV1_ADDR	0x28
#define I2C_SLV1_REG	0x29
#define I2C_SLV1_CTRL	0x2A
// I2C Slave 2 Control
#define I2C_SLV2_ADDR	0x2B
#define I2C_SLV2_REG	0x2C
#define I2C_SLV2_CTRL	0x2D
// I2C Slave 3 Control
#define I2C_SLV3_ADDR	0x2E
#define I2C_SLV3_REG	0x2F
#define I2C_SLV3_CTRL	0x30
// I2C Slave 4 Control
#define I2C_SLV4_ADDR	0x31
#define I2C_SLV4_REG	0x32
#define I2C_SLV4_DO	0x33
#define I2C_SLV4_CTRL	0x34
#define I2C_SLV4_DI	0x35
// I2C Master Status
#define I2C_MST_STATUS	0x36

// INT Pin / Bypass Enable Configuration
#define INT_PIN_CFG	0x37
// Interrupt Enable
#define INT_ENABLE	0x38
// Interrupt Status
#define INT_STATUS	0x3A

// Accelerometer Measurements
#define ACCEL_XOUT_H	0x3B
#define ACCEL_XOUT_L	0x3C
#define ACCEL_YOUT_H	0x3D
#define ACCEL_YOUT_L	0x3E
#define ACCEL_ZOUT_H	0x3F
#define ACCEL_ZOUT_L	0x40

// Temperature Measurement
#define TEMP_OUT_H	0x41
#define TEMP_OUT_L	0x42

// Gyroscope Measurements
#define GYRO_XOUT_H	0x43
#define GYRO_XOUT_L	0x44
#define GYRO_YOUT_H	0x45
#define GYRO_YOUT_L	0x46
#define GYRO_ZOUT_H	0x47
#define GYRO_ZOUT_L	0x48

// External Sensor Data (0 <= i <= 23)
#define EXT_SENS_DATA(i)	(0x49 + (i))

// I2C Slave 0 Data Out
#define I2C_SLV0_DO	0x63
// I2C Slave 1 Data Out
#define I2C_SLV1_DO	0x64
// I2C Slave 2 Data Out
#define I2C_SLV2_DO	0x65
// I2C Slave 3 Data Out
#define I2C_SLV3_DO	0x66
// I2C Master Delay Control
#define I2C_MST_DELAY_CTRL	0x67

// Signal Path Reset
#define SIGNAL_PATH_RESET	0x68
// Accelerometer Interrupt Control
#define MOT_DETECT_CTRL	0x69
#define ACCEL_INTEL_CTRL	0x69
// User Control
#define USER_CTRL	0x6A
// Power Management 1
#define PWR_MGMT_1	0x6B
// Power Management 2
#define PWR_MGMT_2	0x6C
// FIFO Count Registers
#define FIFO_COUNTH	0x72
#define FIFO_COUNTL	0x73
// FIFO Read Write
#define FIFO_R_W	0x74
// Who Am I
#define WHO_AM_I	0x75
// Accelerometer Offset Registers
#define XA_OFFSET_H	0x77
#define XA_OFFSET_L	0x78
#define YA_OFFSET_H	0x7A
#define YA_OFFSET_L	0x7B
#define ZA_OFFSET_H	0x7D
#define ZA_OFFSET_L	0x7E

/* AK8963 Magnetometer */

#define WIA	0x00
#define INFO	0x01
#define ST1	0x02
#define HXL	0x03
#define HXH	0x04
#define HYL	0x05
#define HYH	0x06
#define HZL	0x07
#define HZH	0x08
#define ST2	0x09
#define CNTL	0x0A
#define RSV	0x0B
#define ASTC	0x0C
#define TS1	0x0D
#define TS2	0x0E
#define I2CDIS	0x0F
#define ASAX	0x10
#define ASAY	0x11
#define ASAZ	0x12

#endif // MPU_DEFS_H
