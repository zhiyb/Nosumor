#include <malloc.h>
#include <string.h>
#include <stm32f7xx.h>
#include <system/systick.h>
#include <irq.h>
#include <debug.h>
#include "i2c.h"

struct i2c_node_t {
	struct i2c_node_t *next;
	const struct i2c_op_t *op;
};

enum I2CStatus {Idle = 0, Read, Write, Error = 0x80};

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
	I2C_TypeDef *base = conf->base;
	if (base == I2C1) {
		i2c_p[0] = i2c;
		// Enable NVIC interrupts
		uint32_t pg = NVIC_GetPriorityGrouping();
		NVIC_SetPriority(I2C1_EV_IRQn,
				 NVIC_EncodePriority(pg, NVIC_PRIORITY_I2C, 0));
		NVIC_EnableIRQ(I2C1_EV_IRQn);
	} else
		dbgbkpt();

	// Initialise RX DMA
	// Peripheral to memory, 8bit -> 8bit, burst of 4 beats, low priority
	conf->rx->CR = (conf->rxch << DMA_SxCR_CHSEL_Pos) | (0b00ul << DMA_SxCR_PL_Pos) |
			(0b01ul << DMA_SxCR_MBURST_Pos) | (0b00ul << DMA_SxCR_PBURST_Pos) |
			(0b00ul << DMA_SxCR_MSIZE_Pos) | (0b00ul << DMA_SxCR_PSIZE_Pos) |
			(0b00ul << DMA_SxCR_DIR_Pos) | DMA_SxCR_MINC_Msk;
	// Peripheral address
	conf->rx->PAR = (uint32_t)&base->RXDR;
	// FIFO control
	conf->rx->FCR = DMA_SxFCR_DMDIS_Msk;

	// Initialise TX DMA
	// Memory to peripheral, 8bit -> 8bit, burst of 4 beats, low priority
	conf->tx->CR = (conf->txch << DMA_SxCR_CHSEL_Pos) | (0b00ul << DMA_SxCR_PL_Pos) |
			(0b01ul << DMA_SxCR_MBURST_Pos) | (0b00ul << DMA_SxCR_PBURST_Pos) |
			(0b00ul << DMA_SxCR_MSIZE_Pos) | (0b00ul << DMA_SxCR_PSIZE_Pos) |
			(0b01ul << DMA_SxCR_DIR_Pos) | DMA_SxCR_MINC_Msk;
	// Peripheral address
	conf->tx->PAR = (uint32_t)&base->TXDR;
	// FIFO control
	conf->tx->FCR = DMA_SxFCR_DMDIS_Msk;

	// Disable I2C, disable interrupts
	base->CR1 = 0;
	base->CR2 = 0;
	// I2C at 400kHz (using 54MHz input clock)
	// 100ns SU:DAT, 0us HD:DAT, 0.8us high period, 1.3us low period
	base->TIMINGR = (0u << I2C_TIMINGR_PRESC_Pos) |
			(6u << I2C_TIMINGR_SCLDEL_Pos) | (0u << I2C_TIMINGR_SDADEL_Pos) |
			(44u << I2C_TIMINGR_SCLH_Pos) | (71u << I2C_TIMINGR_SCLL_Pos);
	// No timeouts
	base->TIMEOUTR = 0;
	// Clear interrupt flags
	base->ICR = I2C_ICR_STOPCF_Msk | I2C_ICR_NACKCF_Msk;
	// Enable transfer complete interrupt, enable I2C
	base->CR1 = I2C_CR1_TCIE_Msk | I2C_CR1_STOPIE_Msk | I2C_CR1_PE_Msk;
	systick_delay(2);
	return i2c;
}

static void callback(struct i2c_t *i2c,
		     const struct i2c_op_t *op, uint32_t nack)
{
	i2c->op_status = nack ? Error : Idle;
}

static void callback_nb(struct i2c_t *i2c,
			const struct i2c_op_t *op, uint32_t nack)
{
	__disable_irq();
	free((void *)op);
	__enable_irq();
}

int i2c_check(struct i2c_t *i2c, uint8_t addr)
{
	i2c->op = (struct i2c_op_t){
			.op = I2CCheck, .addr = addr, .reg = 0,
			.p = 0, .size = 0, .cb = callback,
	};
	i2c->op_status = Write;
	i2c_op(i2c, &i2c->op);
	while (i2c->op_status == Write)
		__WFE();
	return !(i2c->op_status & Error);
}

int i2c_write(struct i2c_t *i2c, uint8_t addr, uint8_t reg,
	      const uint8_t *p, uint32_t cnt)
{
	i2c->op = (struct i2c_op_t){
			.op = I2CWrite, .addr = addr, .reg = reg,
			.p = (uint8_t *)p, .size = cnt, .cb = callback,
	};
	i2c->op_status = Write;
	i2c_op(i2c, &i2c->op);
	while (i2c->op_status == Write)
		__WFE();
	return !(i2c->op_status & Error);
}

void i2c_write_nb(struct i2c_t *i2c, uint8_t addr, uint8_t reg,
		 const uint8_t *p, uint32_t cnt, uint32_t dma)
{
	__disable_irq();
	struct i2c_op_t *op = malloc(sizeof(struct i2c_op_t));
	__enable_irq();
	memcpy(op, &(const struct i2c_op_t){
		       .op = I2CWrite | (dma ? I2CDMA : 0),
		       .addr = addr, .reg = reg,
		       .p = (uint8_t *)p, .size = cnt, .cb = callback_nb,
	}, sizeof(struct i2c_op_t));
	i2c_op(i2c, op);
}

int i2c_write_reg(struct i2c_t *i2c, uint8_t addr, uint8_t reg, uint8_t val)
{
	i2c->op = (struct i2c_op_t){
			.op = I2CWrite, .addr = addr, .reg = reg,
			.p = &val, .size = 1, .cb = callback,
	};
	i2c->op_status = Write;
	i2c_op(i2c, &i2c->op);
	while (i2c->op_status == Write)
		__WFE();
	return !(i2c->op_status & Error);
}

void i2c_write_reg_nb(struct i2c_t *i2c, uint8_t addr, uint8_t reg, uint8_t val)
{
	__disable_irq();
	struct i2c_op_t *op = malloc(sizeof(struct i2c_op_t) + 1);
	__enable_irq();
	uint8_t *vp = (uint8_t *)op + sizeof(struct i2c_op_t);
	*vp = val;
	memcpy(op, &(const struct i2c_op_t){
		       .op = I2CWrite, .addr = addr, .reg = reg,
		       .p = vp, .size = 1, .cb = callback_nb,
	}, sizeof(struct i2c_op_t));
	i2c_op(i2c, op);
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
	while (i2c->op_status == Read)
		__WFE();
	return !(i2c->op_status & Error);
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
	while (i2c->op_status == Read)
		__WFE();
	return v;
}

static void op_start(struct i2c_t *i2c, const struct i2c_op_t *op)
{
	I2C_TypeDef *base = i2c->c.base;
	if (op->op == (I2CRead | I2CDMA))
		// Enable reception DMA
		base->CR1 |= I2C_CR1_RXDMAEN_Msk;
	// Critical section, op_done should not be invoked
	if (base == I2C1)
		NVIC_DisableIRQ(I2C1_EV_IRQn);
	else
		dbgbkpt();
	// Start register address write transfer
	if (op->op == I2CCheck) {
		base->CR2 = ((op->addr << 1u) << I2C_CR2_SADD_Pos) |
				(0u << I2C_CR2_NBYTES_Pos) |
				I2C_CR2_START_Msk | I2C_CR2_AUTOEND_Msk;
	} else if ((op->op & (I2CRead | I2CWrite)) == I2CRead) {
		base->CR2 = ((op->addr << 1u) << I2C_CR2_SADD_Pos) |
				I2C_CR2_START_Msk | (1u << I2C_CR2_NBYTES_Pos);
		// Transmit register address
		base->TXDR = op->reg;
		// Enable RX register not empty interrupt
		base->CR1 |= I2C_CR1_RXIE_Msk;
	} else {
		base->CR2 = ((op->addr << 1u) << I2C_CR2_SADD_Pos) |
				((1u + op->size) << I2C_CR2_NBYTES_Pos) |
				I2C_CR2_START_Msk | I2C_CR2_AUTOEND_Msk;
		// Transmit register address
		base->TXDR = op->reg;
		// Enable TX register empty interrupt
		base->CR1 |= I2C_CR1_TXIE_Msk;
	}
	// Update I2C status
	i2c->p = op->p;
	i2c->cnt = op->size;
	i2c->status = Write;
	// Critical section end
	if (base == I2C1)
		NVIC_EnableIRQ(I2C1_EV_IRQn);
	else
		dbgbkpt();
}

static void op_write(struct i2c_t *i2c, const struct i2c_op_t *op)
{
	// Data transmit
	I2C_TypeDef *base = i2c->c.base;
	if (!(op->op & I2CDMA)) {
		base->TXDR = *i2c->p++;
		// Last transfer, disable TX register empty interrupt
		if (!--i2c->cnt)
			base->CR1 &= ~I2C_CR1_TXIE_Msk;
		return;
	}
	DMA_Stream_TypeDef *tx = i2c->c.tx;
	uint32_t n = i2c->cnt;
	uint32_t mask = 0;
	// The I2C peripheral can only transfer 255 bytes maximum
	if (n > 255u) {
		n = 255u;
		mask |= I2C_CR2_RELOAD_Msk;
	}
	// Disable TX register empty interrupt, enable TX DMA
	base->CR1 = (base->CR1 & ~I2C_CR1_TXIE_Msk) | I2C_CR1_TXDMAEN_Msk;
	// Clear DMA flags
	*i2c->c.txr = i2c->c.txm;
	// Setup DMA
	tx->M0AR = (uint32_t)i2c->p;
	tx->NDTR = n;
	// Enable DMA
	tx->CR |= DMA_SxCR_EN_Msk;
	// Start I2C transfer
	base->CR2 = ((op->addr << 1u) << I2C_CR2_SADD_Pos) | mask |
			(n << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND_Msk;
	// Update status
	i2c->p += n;
	i2c->cnt -= n;
	i2c->status = Write;
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

static void op_done(struct i2c_t *i2c, struct i2c_node_t **n, uint32_t nack)
{
	// Detach node
	struct i2c_node_t *node = *n;
	*n = 0;
	// Disable DMAs, disable interrupts
	i2c->c.base->CR1 &= ~(I2C_CR1_TXDMAEN_Msk | I2C_CR1_RXDMAEN_Msk |
			      I2C_CR1_TXIE_Msk | I2C_CR1_RXIE_Msk);
	// Flush transmit data register
	i2c->c.base->ISR = I2C_ISR_TXE_Msk;
	// Start new operation
	i2c->status = Idle;
	op_process(i2c);
	// Callback
	if (node->op->cb)
		node->op->cb(i2c, node->op, nack);
	__disable_irq();
	free(node);
	__enable_irq();
}

void i2c_op(struct i2c_t *i2c, const struct i2c_op_t *op)
{
	__disable_irq();
	struct i2c_node_t *n = malloc(sizeof(struct i2c_node_t));
	n->op = op;
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
	for (n = &i2c->node; n && (*n)->next; n = &(*n)->next);
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
		i2c->c.base->ICR = I2C_ICR_STOPCF_Msk | I2C_ICR_NACKCF_Msk;
		op_done(i2c, n, i & I2C_ISR_NACKF_Msk);
	}
}
