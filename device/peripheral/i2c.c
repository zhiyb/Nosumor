#include <stm32f7xx.h>

void i2c_init(I2C_TypeDef *i2c)
{
	i2c->CR1 = 0;	// Disable I2C, clear interrupts
	i2c->CR2 = 0;
	// I2C at 400kHz (using 54MHz input clock)
	// 100ns SU:DAT, 0us HD:DAT, 0.8us high period, 1.3us low period
	i2c->TIMINGR = (0u << I2C_TIMINGR_PRESC_Pos) |
			(6u << I2C_TIMINGR_SCLDEL_Pos) | (0u << I2C_TIMINGR_SDADEL_Pos) |
			(44u << I2C_TIMINGR_SCLH_Pos) | (71u << I2C_TIMINGR_SCLL_Pos);
	// No timeouts
	i2c->TIMEOUTR = 0;
	// Enable I2C
	i2c->CR1 = I2C_CR1_PE_Msk;
}

int i2c_check(I2C_TypeDef *i2c, uint8_t addr)
{
	i2c->ICR = I2C_ICR_NACKCF_Msk;
	i2c->CR2 = I2C_CR2_AUTOEND_Msk | I2C_CR2_START_Msk |
			(0u << I2C_CR2_NBYTES_Pos) |
			((addr << 1u) << I2C_CR2_SADD_Pos);
	while (i2c->CR2 & I2C_CR2_START_Msk);
	return !(i2c->ISR & I2C_ISR_NACKF_Msk);
}

int i2c_write(I2C_TypeDef *i2c, uint8_t addr, const uint8_t *p, uint32_t cnt)
{
	i2c->CR2 = I2C_CR2_AUTOEND_Msk | I2C_CR2_START_Msk |
			(cnt << I2C_CR2_NBYTES_Pos) |
			((addr << 1u) << I2C_CR2_SADD_Pos);
	while (cnt--) {
		while (!(i2c->ISR & I2C_ISR_TXE_Msk));
		i2c->TXDR = *p++;
	}
	while (i2c->ISR & I2C_ISR_BUSY_Msk);
	return !(i2c->ISR & I2C_ISR_NACKF_Msk);
}

int i2c_write_reg(I2C_TypeDef *i2c, uint8_t addr, uint8_t reg, uint8_t val)
{
	i2c->CR2 = I2C_CR2_AUTOEND_Msk | I2C_CR2_START_Msk |
			(2u << I2C_CR2_NBYTES_Pos) |
			((addr << 1u) << I2C_CR2_SADD_Pos);
	while (!(i2c->ISR & I2C_ISR_TXE_Msk));
	i2c->TXDR = reg;
	while (!(i2c->ISR & I2C_ISR_TXE_Msk));
	i2c->TXDR = val;
	while (i2c->ISR & I2C_ISR_BUSY_Msk);
	return !(i2c->ISR & I2C_ISR_NACKF_Msk);
}

int i2c_read_reg(I2C_TypeDef *i2c, uint8_t addr, uint8_t reg)
{
	i2c->CR2 = I2C_CR2_START_Msk |
			(1u << I2C_CR2_NBYTES_Pos) |
			((addr << 1u) << I2C_CR2_SADD_Pos);
	while (!(i2c->ISR & I2C_ISR_TXE_Msk));
	i2c->TXDR = reg;
	while (i2c->CR2 & I2C_CR2_START_Msk);
	i2c->CR2 = I2C_CR2_AUTOEND_Msk | I2C_CR2_START_Msk | I2C_CR2_RD_WRN_Msk |
			(1u << I2C_CR2_NBYTES_Pos) |
			((addr << 1u) << I2C_CR2_SADD_Pos);
	while (!(i2c->ISR & I2C_ISR_RXNE_Msk));
	return i2c->RXDR;
}