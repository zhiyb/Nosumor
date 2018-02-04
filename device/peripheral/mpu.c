#include <stm32f7xx.h>
#include <macros.h>
#include <debug.h>
#include <system/systick.h>
#include <peripheral/i2c.h>
#include "mpu.h"
#include "mpu_defs.h"

#define I2C_ADDR	MPU_I2C_ADDR

static void *base;
static struct {
	int16_t gyro[3], accel[3];
} data;

static void mpu_tick(uint32_t tick);

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
		// FIFO overrite, FSYNC disabled, DLPH 1
		CONFIG, 0x01,
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
		i2c_write(i2c, I2C_ADDR, p, 2);
		p += 2;
	}
}

uint32_t mpu_init(void *i2c)
{
	if (!i2c_check(i2c, I2C_ADDR))
		return 1;
	base = i2c;
	gpio_init();
	reset(i2c);
	config(i2c);
	return 0;
}

void mpu_process()
{
	uint16_t cnt;
	cnt = i2c_read_reg(base, I2C_ADDR, FIFO_COUNTH) << 8u;
	cnt |= i2c_read_reg(base, I2C_ADDR, FIFO_COUNTL);
	if (!cnt)
		return;
	if (cnt == 512u) {
		// Enable FIFO operation, reset FIFO
		i2c_write_reg(base, I2C_ADDR, USER_CTRL, 0x44);
		dbgprintf(ESC_ERROR "[MPU] FIFO overflow\n");
		return;
	}
	cnt = cnt - cnt % 12u;
	static uint8_t buf[512] SECTION(.dtcm);
	i2c_read(base, I2C_ADDR, FIFO_R_W, buf, cnt);
	// Correct endianness
	uint32_t *u32p = (void *)buf;
	for (uint32_t i = cnt >> 2u; i--; u32p++)
		*u32p = __REV16(*u32p);
	// Data processing
	int16_t *u16p = (void *)buf, *p;
	for (uint32_t i = cnt / 12u; i--;) {
		p = data.accel;
		for (uint32_t j = 3u; j--;)
			*p++ = *u16p++;
		p = data.gyro;
		for (uint32_t j = 3u; j--;)
			*p++ = *u16p++;
	}
}

int16_t *mpu_accel()
{
	return data.accel;
}

int16_t *mpu_gyro()
{
	return data.gyro;
}
