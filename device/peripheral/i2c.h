#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stm32f7xx.h>

#ifdef __cplusplus
extern "C" {
#endif

struct i2c_t;
struct i2c_op_t;

struct i2c_config_t {
	// I2C base address
	I2C_TypeDef *base;
	// DMA streams
	DMA_Stream_TypeDef *rx, *tx;
	// DMA channels
	uint16_t rxch, txch;
	// Flag clear registers
	volatile uint32_t *rxr, *txr;
	// Flag clear masks
	uint32_t rxm, txm;
};

typedef void (*i2c_callback_t)(struct i2c_t *i2c,
			       const struct i2c_op_t *op, uint32_t nack);

enum I2COp {I2CRead = 0, I2CWrite, I2CCheck, I2CDMA = 0x08};

struct i2c_op_t {
	enum I2COp op;
	uint8_t addr, reg;
	void *p;
	uint32_t size;
	i2c_callback_t cb;
};

struct i2c_t *i2c_init(const struct i2c_config_t *conf);
int i2c_check(struct i2c_t *i2c, uint8_t addr);
int i2c_write(struct i2c_t *i2c, uint8_t addr, uint8_t reg,
	      const uint8_t *p, uint32_t cnt);
void i2c_write_nb(struct i2c_t *i2c, uint8_t addr, uint8_t reg,
		 const uint8_t *p, uint32_t cnt, uint32_t dma);
int i2c_write_reg(struct i2c_t *i2c, uint8_t addr, uint8_t reg, uint8_t val);
void i2c_write_reg_nb(struct i2c_t *i2c, uint8_t addr, uint8_t reg, uint8_t val);
int i2c_read(struct i2c_t *i2c, uint8_t addr, uint8_t reg,
	     uint8_t *p, uint32_t cnt);
int i2c_read_reg(struct i2c_t *i2c, uint8_t addr, uint8_t reg);
void i2c_op(struct i2c_t *i2c, const struct i2c_op_t *op);

#ifdef __cplusplus
}
#endif

#endif // I2C_H
