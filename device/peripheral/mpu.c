#include <stm32f7xx.h>
#include <macros.h>
#include <debug.h>
#include <system/systick.h>
#include <peripheral/i2c.h>
#include <api/api_config_priv.h>
#include <usb/hid/usb_hid.h>
#include "mpu.h"
#include "mpu_defs.h"

#define AVG_N	6
#define AVG_NUM	(1u << AVG_N)

#define I2C_ADDR	MPU_I2C_ADDR

static struct {
	struct PACKED {
		int16_t gyro[3], accel[3];
	} log[AVG_NUM], avg;
	struct PACKED {
		int32_t gyro[3], accel[3];
	} sum;
	volatile uint16_t idx;
	volatile uint32_t cnt;
	usb_hid_if_t *hid;
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

uint32_t mpu_init(void *i2c)
{
	if (!i2c_check(i2c, I2C_ADDR))
		return 1;
	data.hid = 0;
	data.idx = 0;
	data.cnt = 0;
	gpio_init();
	reset(i2c);
	config(i2c);
	start(i2c);
	return 0;
}

void mpu_usb_hid(usb_hid_if_t *hid)
{
	data.hid = hid;
}

static void data_callback(struct i2c_t *i2c,
			  const struct i2c_op_t *op, uint32_t nack)
{
	// Correct endianness
	uint32_t *u32p = (void *)op->p;
	for (uint32_t i = op->size >> 2u; i--; u32p++)
		*u32p = __REV16(*u32p);
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
		i32p = data.sum.gyro;
		p = data.log[idx].gyro;
		for (uint32_t j = 3u; j--;) {
			*i32p++ += *i16p - *p;
			*p++ = *i16p++;
		}
	}
	// Update average values
	data.avg.accel[0] = data.sum.accel[0] >> AVG_N;
	data.avg.accel[1] = data.sum.accel[1] >> AVG_N;
	data.avg.accel[2] = data.sum.accel[2] >> AVG_N;
	data.avg.gyro[0] = data.sum.gyro[0] >> AVG_N;
	data.avg.gyro[1] = data.sum.gyro[1] >> AVG_N;
	data.avg.gyro[2] = data.sum.gyro[2] >> AVG_N;
	// Update USB HID report
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
	rp->rx = data.avg.gyro[1];
	rp->ry = data.avg.gyro[0];
	rp->rz = data.avg.gyro[2];
	usb_hid_update(data.hid);
}

static void fifo(struct i2c_t *i2c, uint16_t cnt)
{
	if (cnt == 512u) {
		dbgprintf(ESC_ERROR "[MPU] FIFO overflow\n");
		// Enable FIFO operation, reset FIFO
		i2c_write_reg_nb(i2c, I2C_ADDR, USER_CTRL, 0x44);
		return;
	}
	cnt = cnt - cnt % 12u;
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

volatile int16_t *mpu_accel()
{
	return data.log[data.idx].accel;
}

volatile int16_t *mpu_gyro()
{
	return data.log[data.idx].gyro;
}

volatile int16_t *mpu_accel_avg()
{
	return data.avg.accel;
}

volatile int16_t *mpu_gyro_avg()
{
	return data.avg.gyro;
}

uint32_t mpu_cnt()
{
	return data.cnt;
}
