#include <malloc.h>
#include <stm32f7xx.h>
#include <system/systick.h>
#include <irq.h>
#include <debug.h>
#include "i2c.h"

struct i2c_node_t {
	struct i2c_node_t *next;
	const struct i2c_op_t *op;
};

enum I2CStatus {Idle = 0, Read, Write};

struct i2c_t {
	struct i2c_config_t c;
	struct i2c_node_t *node;
	uint8_t *p;
	uint32_t cnt;
	volatile enum I2CStatus status, op_status;
	struct i2c_op_t op;
} *i2c_p[1];

struct i2c_t *i2c_init(const struct i2c_config_t *conf)
{
	struct i2c_t *i2c = malloc(sizeof(struct i2c_t));
	i2c->c = *conf;
	i2c->node = 0;
	i2c->status = Idle;
	if (conf->base == I2C1) {
		i2c_p[0] = i2c;
		// Enable NVIC interrupts
		uint32_t pg = NVIC_GetPriorityGrouping();
		NVIC_SetPriority(I2C1_EV_IRQn,
				 NVIC_EncodePriority(pg, NVIC_PRIORITY_I2C, 0));
		NVIC_EnableIRQ(I2C1_EV_IRQn);
	}

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

static void callback(struct i2c_t *i2c, const struct i2c_op_t *op)
{
	i2c->op_status = Idle;
}

int i2c_check(struct i2c_t *i2c, uint8_t addr)
{
	return 1;
	while (i2c->status != Idle);
	I2C_TypeDef *base = i2c->c.base;
	base->ICR = I2C_ICR_STOPCF_Msk | I2C_ICR_NACKCF_Msk;
	base->CR2 = I2C_CR2_AUTOEND_Msk | I2C_CR2_START_Msk |
			(0u << I2C_CR2_NBYTES_Pos) |
			((addr << 1u) << I2C_CR2_SADD_Pos);
	while (!(base->ISR & I2C_ISR_STOPF_Msk));
	base->ICR = I2C_ICR_STOPCF_Msk;
	return !(base->ISR & I2C_ISR_NACKF_Msk);
}

int i2c_write(struct i2c_t *i2c, uint8_t addr, const uint8_t *p, uint32_t cnt)
{
	i2c->op = (struct i2c_op_t){
			.op = I2CWrite, .addr = addr, .reg = *p,
			.p = (uint8_t *)p + 1, .size = cnt - 1, .cb = callback,
	};
	i2c->op_status = Write;
	i2c_op(i2c, &i2c->op);
	while (i2c->op_status != Idle);
	return 1;
}

int i2c_write_reg(struct i2c_t *i2c, uint8_t addr, uint8_t reg, uint8_t val)
{
	i2c->op = (struct i2c_op_t){
			.op = I2CWrite, .addr = addr, .reg = reg,
			.p = &val, .size = 1, .cb = callback,
	};
	i2c->op_status = Write;
	i2c_op(i2c, &i2c->op);
	while (i2c->op_status != Idle);
	return 1;
}

int i2c_read(struct i2c_t *i2c, uint8_t addr, uint8_t reg,
	     uint8_t *p, uint32_t cnt)
{
	i2c->op = (struct i2c_op_t){
			.op = I2CRead, .addr = addr, .reg = reg,
			.p = p, .size = cnt, .cb = callback,
	};
	i2c->op_status = Read;
	i2c_op(i2c, &i2c->op);
	while (i2c->op_status != Idle);
	return 1;
}

int i2c_read_reg(struct i2c_t *i2c, uint8_t addr, uint8_t reg)
{
	uint8_t v;
	i2c->op = (struct i2c_op_t){
			.op = I2CRead, .addr = addr, .reg = reg,
			.p = &v, .size = 1, .cb = callback,
	};
	i2c->op_status = Read;
	i2c_op(i2c, &i2c->op);
	while (i2c->op_status != Idle);
	return v;
}

static void op_start(struct i2c_t *i2c, const struct i2c_op_t *op)
{
	I2C_TypeDef *base = i2c->c.base;
	// Wait for transfer complete
	base->CR1 |= I2C_CR1_TCIE_Msk | I2C_CR1_STOPIE_Msk;
	if (op->op == (I2CRead | I2CDMA))
		// Enable reception DMA
		base->CR1 |= I2C_CR1_RXDMAEN_Msk;
	else
		// Enable RX register not empty interrupt
		base->CR1 |= I2C_CR1_RXIE_Msk;
	// Start register address write transfer
	if ((op->op & (I2CRead | I2CWrite)) == I2CRead) {
		base->CR2 = ((op->addr << 1u) << I2C_CR2_SADD_Pos) |
				I2C_CR2_START_Msk | (1u << I2C_CR2_NBYTES_Pos);
	} else {
		base->CR2 = ((op->addr << 1u) << I2C_CR2_SADD_Pos) |
				((1u + op->size) << I2C_CR2_NBYTES_Pos) |
				I2C_CR2_START_Msk | I2C_CR2_AUTOEND_Msk;
		// Enable TX register empty interrupt
		base->CR1 |= I2C_CR1_TXIE_Msk;
	}
	i2c->p = op->p;
	i2c->cnt = op->size;
	i2c->status = Write;
	base->TXDR = op->reg;
}

static void op_write(struct i2c_t *i2c, const struct i2c_op_t *op)
{
	// Data transmit
	I2C_TypeDef *base = i2c->c.base;
	DMA_Stream_TypeDef *tx = i2c->c.tx;
	uint32_t n = i2c->cnt;
	uint32_t mask = 0;
	if (op->op & I2CDMA)
		dbgbkpt();
	base->TXDR = *i2c->p++;
	// Last transfer, disable TX register empty interrupt
	if (!--i2c->cnt)
		base->CR1 &= ~I2C_CR1_TXIE_Msk;
}

static void op_read(struct i2c_t *i2c, const struct i2c_op_t *op)
{
	// Data reception
	I2C_TypeDef *base = i2c->c.base;
	if (!(op->op & I2CDMA)) {
		// Restart if necessary
		if (i2c->status == Write) {
			// Start I2C transfer
			base->CR2 = ((op->addr << 1u) << I2C_CR2_SADD_Pos) |
					(i2c->cnt << I2C_CR2_NBYTES_Pos) |
					I2C_CR2_START_Msk | I2C_CR2_AUTOEND_Msk |
					I2C_CR2_RD_WRN_Msk;
			i2c->status = Read;
		} else {
			*i2c->p++ = base->RXDR;
			// Last transfer, disable RX register empty interrupt
			if (!--i2c->cnt)
				base->CR1 &= ~I2C_CR1_RXIE_Msk;
		}
		return;
	}
	DMA_Stream_TypeDef *rx = i2c->c.rx;
	// The I2C peripheral can only transfer 255 bytes maximum
	uint32_t n = i2c->cnt;
	// Restart if necessary
	uint32_t mask = i2c->status == Write ? I2C_CR2_START_Msk : 0;
	if (n > 255u) {
		n = 255u;
		mask |= I2C_CR2_RELOAD_Msk;
	}
	// Clear DMA flags
	*i2c->c.rxr = i2c->c.rxm;
	// Setup DMA
	rx->M0AR = (uint32_t)i2c->p;
	rx->NDTR = n;
	// Enable DMA
	rx->CR |= DMA_SxCR_EN_Msk;
	// Start I2C transfer
	base->CR2 = ((op->addr << 1u) << I2C_CR2_SADD_Pos) | mask |
			(n << I2C_CR2_NBYTES_Pos) |
			I2C_CR2_AUTOEND_Msk | I2C_CR2_RD_WRN_Msk;
	// Update status
	i2c->p += n;
	i2c->cnt -= n;
	i2c->status = Read;
	mask = (mask & I2C_CR2_RELOAD_Msk) ?
				I2C_CR1_TCIE_Msk : I2C_CR1_STOPIE_Msk;
	base->CR1 = (base->CR1 & ~I2C_CR1_TCIE_Msk) | mask;
}

static void op_process(struct i2c_t *i2c)
{
	if (i2c->status != Idle)
		return;
	if (!i2c->node)
		return;
	// Find the earliest node
	struct i2c_node_t **n;
	for (n = &i2c->node; (*n)->next; n = &(*n)->next);
	// Process op
	const struct i2c_op_t *op = (*n)->op;
	op_start(i2c, op);
}

static void op_done(struct i2c_t *i2c, struct i2c_node_t **n)
{
	// Detach node
	struct i2c_node_t *node = *n;
	*n = 0;
	// Disable DMAs, disable interrupts
	i2c->c.base->CR1 &= ~(I2C_CR1_TXDMAEN_Msk | I2C_CR1_RXDMAEN_Msk |
			      I2C_CR1_TXIE_Msk | I2C_CR1_RXIE_Msk);
	// Start new operation
	i2c->status = Idle;
	op_process(i2c);
	// Callback
	if (node->op->cb)
		node->op->cb(i2c, node->op);
	free(node);
}

void i2c_op(struct i2c_t *i2c, const struct i2c_op_t *op)
{
	struct i2c_node_t *n = malloc(sizeof(struct i2c_node_t));
	n->op = op;
	__disable_irq();
	n->next = i2c->node;
	i2c->node = n;
	__enable_irq();
	op_process(i2c);
}

void I2C1_EV_IRQHandler()
{
	struct i2c_t *i2c = i2c_p[0];
	// Find the earliest node
	struct i2c_node_t **n;
	for (n = &i2c->node; (*n)->next; n = &(*n)->next);
	const struct i2c_op_t *op = (*n)->op;
	// Process interrupts
	uint32_t i = I2C1->ISR &
			((I2C1->CR1 & (I2C_CR1_RXIE_Msk | I2C_CR1_TXIE_Msk)) |
			 ~(I2C_CR1_RXIE_Msk | I2C_CR1_TXIE_Msk));
	if (i & I2C_ISR_TXIS_Msk) {
		op_write(i2c, op);
	} else if (i & I2C_ISR_RXNE_Msk) {
		op_read(i2c, op);
	} else if (i & I2C_ISR_TCR_Msk) {
		if (i2c->status == Read)
			// Data reception reload
			op_read(i2c, op);
		else
			op_write(i2c, op);
	} else if (i & I2C_ISR_TC_Msk) {
		// Only read operation will trigger transfer complete interrupt
		// Register address wrote
		op_read(i2c, op);
	} else if (i & I2C_ISR_STOPF_Msk) {
		i2c->c.base->ICR = I2C_ICR_STOPCF_Msk;
		op_done(i2c, n);
	}
}
