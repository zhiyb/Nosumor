#include <malloc.h>
#include <stm32f7xx.h>
#include <system/systick.h>
#include <debug.h>
#include "i2c.h"

struct i2c_t {
	struct i2c_config_t c;
};

struct i2c_t *i2c_init(const struct i2c_config_t *conf)
{
	struct i2c_t *i2c = malloc(sizeof(struct i2c_t));
	i2c->c = *conf;

	conf->base->CR1 = 0;	// Disable I2C, clear interrupts
	conf->base->CR2 = 0;
	// I2C at 400kHz (using 54MHz input clock)
	// 100ns SU:DAT, 0us HD:DAT, 0.8us high period, 1.3us low period
	conf->base->TIMINGR = (0u << I2C_TIMINGR_PRESC_Pos) |
			(6u << I2C_TIMINGR_SCLDEL_Pos) | (0u << I2C_TIMINGR_SDADEL_Pos) |
			(44u << I2C_TIMINGR_SCLH_Pos) | (71u << I2C_TIMINGR_SCLL_Pos);
	// No timeouts
	conf->base->TIMEOUTR = 0;
	// Enable I2C
	conf->base->CR1 = I2C_CR1_PE_Msk;

	// Peripheral to memory, 8bit -> 8bit, burst of 4 beats, low priority
	conf->rx->CR = (conf->rxch << DMA_SxCR_CHSEL_Pos) | (0b00ul << DMA_SxCR_PL_Pos) |
			(0b01ul << DMA_SxCR_MBURST_Pos) | (0b00ul << DMA_SxCR_PBURST_Pos) |
			(0b00ul << DMA_SxCR_MSIZE_Pos) | (0b00ul << DMA_SxCR_PSIZE_Pos) |
			(0b00ul << DMA_SxCR_DIR_Pos) | DMA_SxCR_MINC_Msk;
	// Peripheral address
	conf->rx->PAR = (uint32_t)&conf->base->RXDR;
	// FIFO control
	conf->rx->FCR = DMA_SxFCR_DMDIS_Msk;
	return i2c;
}

int i2c_check(struct i2c_t *i2c, uint8_t addr)
{
	I2C_TypeDef *base = i2c->c.base;
	base->ICR = I2C_ICR_STOPCF_Msk | I2C_ICR_NACKCF_Msk;
	base->CR2 = I2C_CR2_AUTOEND_Msk | I2C_CR2_START_Msk |
			(0u << I2C_CR2_NBYTES_Pos) |
			((addr << 1u) << I2C_CR2_SADD_Pos);
	while (!(base->ISR & I2C_ISR_STOPF_Msk));
	return !(base->ISR & I2C_ISR_NACKF_Msk);
}

int i2c_write(struct i2c_t *i2c, uint8_t addr, const uint8_t *p, uint32_t cnt)
{
	I2C_TypeDef *base = i2c->c.base;
	base->ICR = I2C_ICR_STOPCF_Msk | I2C_ICR_NACKCF_Msk;
	base->CR2 = I2C_CR2_AUTOEND_Msk | I2C_CR2_START_Msk |
			(cnt << I2C_CR2_NBYTES_Pos) |
			((addr << 1u) << I2C_CR2_SADD_Pos);
	while (cnt--) {
		while (!(base->ISR & I2C_ISR_TXE_Msk));
		base->TXDR = *p++;
	}
	while (!(base->ISR & I2C_ISR_STOPF_Msk));
	return !(base->ISR & I2C_ISR_NACKF_Msk);
}

int i2c_write_reg(struct i2c_t *i2c, uint8_t addr, uint8_t reg, uint8_t val)
{
	I2C_TypeDef *base = i2c->c.base;
	base->ICR = I2C_ICR_STOPCF_Msk | I2C_ICR_NACKCF_Msk;
	base->CR2 = I2C_CR2_AUTOEND_Msk | I2C_CR2_START_Msk |
			(2u << I2C_CR2_NBYTES_Pos) |
			((addr << 1u) << I2C_CR2_SADD_Pos);
	while (!(base->ISR & I2C_ISR_TXE_Msk));
	base->TXDR = reg;
	while (!(base->ISR & I2C_ISR_TXE_Msk));
	base->TXDR = val;
	while (!(base->ISR & I2C_ISR_STOPF_Msk));
	return !(base->ISR & I2C_ISR_NACKF_Msk);
}

int i2c_read(struct i2c_t *i2c, uint8_t addr, uint8_t reg,
	     uint8_t *p, uint32_t cnt)
{
	I2C_TypeDef *base = i2c->c.base;
	DMA_Stream_TypeDef *rx = i2c->c.rx;
	uint32_t addrm = ((addr << 1u) << I2C_CR2_SADD_Pos);
	base->CR2 = addrm | I2C_CR2_START_Msk | (1u << I2C_CR2_NBYTES_Pos);
	while (!(base->ISR & I2C_ISR_TXE_Msk));
	base->TXDR = reg;
	while (!(base->ISR & I2C_ISR_TC_Msk));
	// Enable DMA reception mode
	base->CR1 |= I2C_CR1_RXDMAEN_Msk;
	// Clear I2C flags
	base->ICR = I2C_ICR_STOPCF_Msk | I2C_ICR_NACKCF_Msk;
	uint32_t mask = I2C_CR2_START_Msk;
	while (cnt) {
		uint32_t n = cnt;
		if (cnt > 255u) {
			n = 255u;
			mask |= I2C_CR2_RELOAD_Msk;
		} else
			mask |= I2C_CR2_AUTOEND_Msk;
		// Clear DMA flags
		*i2c->c.rxr = i2c->c.rxm;
		// Setup DMA
		rx->M0AR = (uint32_t)p;
		rx->NDTR = n;
		// Enable DMA
		rx->CR |= DMA_SxCR_EN_Msk;
		// Start I2C transfer
		base->CR2 = addrm | mask | (n << I2C_CR2_NBYTES_Pos) |
				I2C_CR2_RD_WRN_Msk;
		cnt -= n;
		p += n;
		mask = (mask & I2C_CR2_RELOAD_Msk) ?
					I2C_ISR_TCR_Msk : I2C_ISR_STOPF_Msk;
		while (!(base->ISR & mask));
		mask = 0;
	}
	base->CR1 &= ~I2C_CR1_RXDMAEN_Msk;
	return 0;
}

int i2c_read_reg(struct i2c_t *i2c, uint8_t addr, uint8_t reg)
{
	I2C_TypeDef *base = i2c->c.base;
	base->CR2 = I2C_CR2_START_Msk |
			(1u << I2C_CR2_NBYTES_Pos) |
			((addr << 1u) << I2C_CR2_SADD_Pos);
	while (!(base->ISR & I2C_ISR_TXE_Msk));
	base->TXDR = reg;
	while (!(base->ISR & I2C_ISR_TC_Msk));
	base->ICR = I2C_ICR_STOPCF_Msk | I2C_ICR_NACKCF_Msk;
	base->CR2 = I2C_CR2_AUTOEND_Msk | I2C_CR2_START_Msk | I2C_CR2_RD_WRN_Msk |
			(1u << I2C_CR2_NBYTES_Pos) |
			((addr << 1u) << I2C_CR2_SADD_Pos);
	while (!(base->ISR & I2C_ISR_STOPF_Msk));
	return base->RXDR;
}
