#include <math.h>
#include <stm32f7xx.h>
#include <macros.h>
#include <debug.h>
#include <system/systick.h>
#include <peripheral/i2c.h>
#include <api/api_config_priv.h>
#include <usb/hid/usb_hid.h>
#include <inv_mpu.h>
#include <inv_mpu_dmp_motion_driver.h>
#include "mpu.h"
#include "mpu_defs.h"

#define AVG_N	6
#define AVG_NUM	(1u << AVG_N)

#define PKT_SIZE	16u

#define I2C_ADDR	MPU_I2C_ADDR

static struct {
#if 0
	struct PACKED {
		int16_t accel[3];
	} log[AVG_NUM], avg;
	struct PACKED {
		int32_t accel[3];
	} sum;
	struct {
		int32_t sum[3], prev[3], mouse[3];
	} gyro;
#endif
	int32_t quat[4];
	volatile uint16_t idx;
	volatile uint32_t cnt;
	usb_hid_if_t *hid, *hid_mouse;
} data;

static void start(void *i2c);

static void gpio_init()
{
	// Initialise interrupt GPIO (PB15)
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN_Msk;
	GPIO_MODER(GPIOB, 15, 0b00);	// 00: Input mode
	GPIO_PUPDR(GPIOB, 15, GPIO_PUPDR_UP);
}

static void reset(void *i2c)
{
	// FIFO_RST, SIG_COND_RST
	i2c_write_reg(i2c, I2C_ADDR, USER_CTRL, 0x05);
	while (i2c_read_reg(i2c, I2C_ADDR, USER_CTRL) != 0x00);
}

static void config(void *i2c)
{
	static const uint8_t cmd[] = {
		PWR_MGMT_1, 0x00,
		PWR_MGMT_2, 0x00,
		// Data output rate divider
		SMPLRT_DIV, 0x00,
		// Disable FIFO overwrite, FSYNC disabled, DLPH 1
		CONFIG, 0x41,
		// Scale +250dps, Fchoice 0b11 (1kHz)
		GYRO_CONFIG, 0x00,
		// Scale ±2g
		ACCEL_CONFIG, 0x00,
		// Fchoice 1, DLPH 1 (1kHz)
		ACCEL_CONFIG_2, 0x01,
		// FIFO enable: GYRO, ACCEL
		FIFO_EN, 0x78,
		// Enable FIFO operation, reset FIFO
		USER_CTRL, 0x44,
	}, *p = cmd;

	// Write configration sequence
	for (uint32_t i = 0; i != sizeof(cmd) / sizeof(cmd[0]) / 2; i++) {
		i2c_write_reg(i2c, I2C_ADDR, *p, *(p + 1));
		p += 2;
	}
}

uint32_t mpu_sys_init(void *i2c)
{
	if (!i2c_check(i2c, I2C_ADDR))
		return 1;
	data.hid = 0;
	data.idx = 0;
	data.cnt = 0;
	gpio_init();

	int ret;
	if ((ret = mpu_init(0)) != 0) {
		dbgprintf(ESC_ERROR "[MPU] %u: %d\n", __LINE__, ret);
		return ret;
	}
#if 0
	long gyro[3], accel[3];
	ret = mpu_run_6500_self_test(gyro, accel, 1);
	dbgprintf(ESC_INFO "[MPU] self_test: " ESC_DATA "%x, ", ret);
	dbgprintf("(%ld, %ld, %ld), ", gyro[0], gyro[1], gyro[2]);
	dbgprintf("(%ld, %ld, %ld)\n", accel[0], accel[1], accel[2]);
#endif
	if ((ret = mpu_set_sensors(INV_XYZ_GYRO |
				   INV_XYZ_ACCEL |
				   INV_XYZ_COMPASS)) != 0) {
		dbgprintf(ESC_ERROR "[MPU] %u: %d\n", __LINE__, ret);
		return ret;
	}
	if ((ret = dmp_load_motion_driver_firmware()) != 0) {
		dbgprintf(ESC_ERROR "[MPU] %u: %d\n", __LINE__, ret);
		return ret;
	}
	if ((ret = dmp_set_fifo_rate(200)) != 0) {
		dbgprintf(ESC_ERROR "[MPU] %u: %d\n", __LINE__, ret);
		return ret;
	}
	if ((ret = dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT |
				      DMP_FEATURE_GYRO_CAL)) != 0) {
		dbgprintf(ESC_ERROR "[MPU] %u: %d\n", __LINE__, ret);
		return ret;
	}
	if ((ret = mpu_set_dmp_state(1))) {
		dbgprintf(ESC_ERROR "[MPU] %u: %d\n", __LINE__, ret);
		return ret;
	}
#if 0
	for (;;) {
		short gyro[3], accel[3];
		long quat[4];
		unsigned long ts;
		short sensors;
		unsigned char more;
		if (dmp_read_fifo(gyro, accel, quat, &ts, &sensors, &more) != 0)
			continue;
		dbgprintf(ESC_INFO "[MPU] dmp_read @%ld(%u): " ESC_DATA "%x, ",
			  ts, more, sensors);
		if (sensors & INV_XYZ_GYRO)
			dbgprintf("gyro: (%d, %d, %d), ",
				  gyro[0], gyro[1], gyro[2]);
		if (sensors & INV_XYZ_ACCEL)
			dbgprintf("accel: (%d, %d, %d), ",
				  accel[0], accel[1], accel[2]);
		if (sensors & INV_WXYZ_QUAT)
			dbgprintf("quat: (%ld, %ld, %ld, %ld), ",
				  quat[0], quat[1], quat[2], quat[3]);
		dbgprintf("\n");
	}
#endif
#if 0
	reset(i2c);
	config(i2c);
#endif
	start(i2c);
	return 0;
}

void mpu_usb_hid(usb_hid_if_t *hid, usb_hid_if_t *hid_mouse)
{
	data.hid = hid;
	data.hid_mouse = hid_mouse;
}

static void data_callback(struct i2c_t *i2c,
			  const struct i2c_op_t *op, uint32_t nack)
{
	// Correct endianness
	uint32_t *u32p = (void *)op->p;
	for (uint32_t i = op->size >> 2u; i--; u32p++)
		*u32p = __REV(*u32p);
	uint16_t cnt = op->size / PKT_SIZE;
	for (uint32_t i = cnt; i--;)
		memcpy(data.quat, op->p, 16u);
#if 0
	// Data processing
	int32_t *i32p;
	int16_t *i16p = (void *)op->p, *p;
	uint16_t idx = (data.idx + 1u) & (AVG_NUM - 1u);
	uint16_t cnt = op->size / 12u;
	data.idx = idx;
	data.cnt += cnt;
	for (uint32_t i = cnt; i--;) {
		// Update sums, update logged values
		i32p = data.sum.accel;
		p = data.log[idx].accel;
		for (uint32_t j = 3u; j--;) {
			*i32p++ += *i16p - *p;
			*p++ = *i16p++;
		}
		i32p = data.gyro.sum;
		for (uint32_t j = 3u; j--;)
			*i32p++ += *i16p++;
	}
	// Update average values
	data.avg.accel[0] = data.sum.accel[0] >> AVG_N;
	data.avg.accel[1] = data.sum.accel[1] >> AVG_N;
	data.avg.accel[2] = data.sum.accel[2] >> AVG_N;
	// Calculate gyro difference
	int32_t gyro_diff[3] = {
		data.gyro.sum[0] - data.gyro.prev[0],
		data.gyro.sum[1] - data.gyro.prev[1],
		data.gyro.sum[2] - data.gyro.prev[2],
	};
	memcpy(data.gyro.prev, data.gyro.sum, sizeof(data.gyro.sum));
	const int32_t scale = 4096;
	int32_t gyro_mouse[3] = {
		(data.gyro.sum[0] - data.gyro.mouse[0]) / scale,
		(data.gyro.sum[1] - data.gyro.mouse[1]) / scale,
		(data.gyro.sum[2] - data.gyro.mouse[2]) / scale,
	};
	data.gyro.mouse[0] += gyro_mouse[0] * scale;
	data.gyro.mouse[1] += gyro_mouse[1] * scale;
	data.gyro.mouse[2] += gyro_mouse[2] * scale;
	// Update USB HID mouse report
	if (!data.hid_mouse)
		goto js;
	if (!api_config_data.mouse)
		goto js;
	int8_t *ap = (void *)&data.hid_mouse->report.payload[1];
	*ap = gyro_mouse[0];
	*++ap = -gyro_mouse[1];
	usb_hid_update(data.hid_mouse);
js:	// Update USB HID joystick report
	if (!data.hid)
		return;
	if (!api_config_data.joystick_mpu)
		return;
	struct PACKED {
		int16_t x, y, z;
		int16_t rx, ry, rz;
	} *rp = (void *)data.hid->report.payload;
	rp->x = data.avg.accel[1];
	rp->y = data.avg.accel[0];
	rp->z = data.avg.accel[2];
	rp->rx = gyro_diff[1] > 32767 ? 32767 : gyro_diff[1] <= -32768 ? -32768 : gyro_diff[1];
	rp->ry = gyro_diff[0] > 32767 ? 32767 : gyro_diff[0] <= -32768 ? -32768 : gyro_diff[0];
	rp->rz = gyro_diff[2] > 32767 ? 32767 : gyro_diff[2] <= -32768 ? -32768 : gyro_diff[2];
	usb_hid_update(data.hid);
#endif
}

static void fifo(struct i2c_t *i2c, uint16_t cnt)
{
	if (cnt == 512u) {
		dbgprintf(ESC_ERROR "[MPU] FIFO overflow\n");
		// Enable FIFO operation, reset FIFO
		i2c_write_reg_nb(i2c, I2C_ADDR, USER_CTRL, 0x44);
		return;
	}
	cnt = cnt - cnt % PKT_SIZE;
	if (!cnt)
		return;
	static uint8_t buf[512] SECTION(.dtcm);
	static struct i2c_op_t op = {
		.op = I2CRead | I2CDMA, .addr = I2C_ADDR, .reg = FIFO_R_W,
		.p = buf, .size = 0, .cb = data_callback,
	};
	op.size = cnt;
	i2c_op(i2c, &op);
}

static void cnt_callback(struct i2c_t *i2c,
			 const struct i2c_op_t *op, uint32_t nack)
{
	fifo(i2c, *(uint16_t *)op->p);
	// Check FIFO level
	start(i2c);
}

static void start(void *i2c)
{
	static uint16_t cnt SECTION(.dtcm);
	static const struct i2c_op_t op_h = {
		.op = I2CRead, .addr = I2C_ADDR, .reg = FIFO_COUNTH,
		.p = (uint8_t *)&cnt + 1, .size = 1, .cb = 0,
	}, op_l = {
		.op = I2CRead, .addr = I2C_ADDR, .reg = FIFO_COUNTL,
		.p = (uint8_t *)&cnt + 0, .size = 1, .cb = cnt_callback,
	};
	i2c_op(i2c, &op_h);
	i2c_op(i2c, &op_l);
}

#if 0
volatile int16_t *mpu_accel()
{
	return data.log[data.idx].accel;
}

volatile int32_t *mpu_gyro()
{
	return data.gyro.sum;
}

volatile int16_t *mpu_accel_avg()
{
	return data.avg.accel;
}
#endif

volatile int32_t *mpu_quat()
{
	return data.quat;
}

uint32_t mpu_cnt()
{
	return data.cnt;
}
