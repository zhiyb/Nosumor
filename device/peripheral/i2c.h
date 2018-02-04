#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stm32f7xx.h>

#ifdef __cplusplus
extern "C" {
#endif

void i2c_init(I2C_TypeDef *i2c);
int i2c_check(I2C_TypeDef *i2c, uint8_t addr);
int i2c_write(I2C_TypeDef *i2c, uint8_t addr, const uint8_t *p, uint32_t cnt);
int i2c_write_reg(I2C_TypeDef *i2c, uint8_t addr, uint8_t reg, uint8_t val);
int i2c_read(I2C_TypeDef *i2c, uint8_t addr, uint8_t reg,
	     uint8_t *p, uint32_t cnt);
int i2c_read_reg(I2C_TypeDef *i2c, uint8_t addr, uint8_t reg);

#ifdef __cplusplus
}
#endif

#endif // I2C_H
