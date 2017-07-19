#include <stdio.h>
#include <math.h>
#include <stm32f7xx.h>
#include "../macros.h"
#include "../escape.h"
#include "../debug.h"
#include "../systick.h"
#include "audio.h"

#define I2C		I2C1
#define I2C_ADDR	0b0011000u

static int i2c_check(I2C_TypeDef *i2c, uint8_t addr);
static void audio_reset();
static void audio_test();

void audio_init()
{
	// Initialise I2C GPIOs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN_Msk;
	GPIO_MODER(GPIOB, 8, 0b10);	// 10: Alternative function mode
	GPIO_OTYPER_OD(GPIOB, 8);
	GPIO_OSPEEDR(GPIOB, 8, 0b00);	// Low speed (4MHz)
	GPIO_PUPDR(GPIOB, 8, GPIO_PUPDR_UP);
	GPIO_AFRH(GPIOB, 8, 4);		// AF4: I2C1

	GPIO_MODER(GPIOB, 9, 0b10);
	GPIO_OTYPER_OD(GPIOB, 9);
	GPIO_OSPEEDR(GPIOB, 9, 0b00);
	GPIO_PUPDR(GPIOB, 9, GPIO_PUPDR_UP);
	GPIO_AFRH(GPIOB, 9, 4);

	// Initialise I2C module
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN_Msk;
	I2C->CR1 = 0;	// Disable I2C, clear interrupts
	I2C->CR2 = 0;
	// I2C at 400kHz (using 54MHz input clock)
	// 100ns SU:DAT, 0us HD:DAT, 0.8us high period, 1.3us low period
	I2C->TIMINGR = (0u << I2C_TIMINGR_PRESC_Pos) |
			(6u << I2C_TIMINGR_SCLDEL_Pos) | (0u << I2C_TIMINGR_SDADEL_Pos) |
			(44u << I2C_TIMINGR_SCLH_Pos) | (71u << I2C_TIMINGR_SCLL_Pos);
	// No timeouts
	I2C->TIMEOUTR = 0;
	// Enable I2C
	I2C->CR1 = I2C_CR1_PE_Msk;

	if (!i2c_check(I2C, I2C_ADDR)) {
		dbgbkpt();
		return;
	}

	// Software reset
	audio_reset();
	// Waiting for reset
	systick_delay(10);

	// Initialise I2S GPIO
	// I2S1 (slave transmit): WS(PA4), DIN(PA7), CK(PB3)
	// I2S2 (slave receive): WS(PB4), DOUT(PC1), CK(PA9)
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN_Msk | RCC_AHB1ENR_GPIOBEN_Msk | RCC_AHB1ENR_GPIOCEN_Msk;
	// PA4
	GPIO_MODER(GPIOA, 4, 0b10);	// 10: Alternative function mode
	GPIO_AFRL(GPIOA, 4, 5);		// AF5: I2S1
	// PA7
	GPIO_MODER(GPIOA, 7, 0b10);
	GPIO_OTYPER_PP(GPIOA, 7);
	GPIO_OSPEEDR(GPIOA, 7, 0b01);	// Medium speed (25MHz)
	GPIO_AFRL(GPIOA, 7, 5);
	// PA9
	GPIO_MODER(GPIOA, 9, 0b10);
	GPIO_AFRH(GPIOA, 9, 5);
	// PB3
	GPIO_MODER(GPIOB, 3, 0b10);
	GPIO_AFRL(GPIOB, 3, 5);
	// PB4
	GPIO_MODER(GPIOB, 4, 0b10);
	GPIO_AFRL(GPIOB, 4, 7);
	// PC1
	GPIO_MODER(GPIOC, 1, 0b10);
	GPIO_AFRL(GPIOC, 1, 5);

	// Initialise I2S modules
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN_Msk;
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN_Msk;
	// I2S1: Slave transmit, 32-bit
	SPI1->CR1 = 0;
	SPI1->CR2 = 0;
	SPI1->I2SCFGR = SPI_I2SCFGR_I2SMOD_Msk | SPI_I2SCFGR_CHLEN_Msk |
			(0b00u << SPI_I2SCFGR_I2SCFG_Pos) |
			(0b10u << SPI_I2SCFGR_DATLEN_Pos);
	// I2S2: Slave receive, 32-bit
	SPI2->CR1 = 0;
	SPI2->CR2 = 0;
	SPI2->I2SCFGR = SPI_I2SCFGR_I2SMOD_Msk | SPI_I2SCFGR_CHLEN_Msk |
			(0b01u << SPI_I2SCFGR_I2SCFG_Pos) |
			(0b10u << SPI_I2SCFGR_DATLEN_Pos);
	// Enable I2S audio interfaces
	SPI1->I2SCFGR |= SPI_I2SCFGR_I2SE_Msk;
	SPI2->I2SCFGR |= SPI_I2SCFGR_I2SE_Msk;

	// Codec GPIO (interrupt): PC4
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN_Msk;
	GPIO_MODER(GPIOC, 4, 0b00);	// 00: Input mode
	GPIO_PUPDR(GPIOC, 4, GPIO_PUPDR_UP);

	// Tests
	audio_test();
}

static int i2c_check(I2C_TypeDef *i2c, uint8_t addr)
{
	i2c->ICR = I2C_ICR_NACKCF_Msk;
	i2c->CR2 = I2C_CR2_AUTOEND_Msk | I2C_CR2_START_Msk |
			(0u << I2C_CR2_NBYTES_Pos) |
			((addr << 1u) << I2C_CR2_SADD_Pos);
	while (i2c->CR2 & I2C_CR2_START_Msk);
	return !(i2c->ISR & I2C_ISR_NACKF_Msk);
}

static int i2c_write(I2C_TypeDef *i2c, uint8_t addr, const uint8_t *p, uint32_t cnt)
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

static int i2c_write_reg(I2C_TypeDef *i2c, uint8_t addr, uint8_t reg, uint8_t val)
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

static int i2c_read_reg(I2C_TypeDef *i2c, uint8_t addr, uint8_t reg)
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

static void audio_page(I2C_TypeDef *i2c, uint8_t page)
{
	i2c_write_reg(i2c, I2C_ADDR, 0x00, page);
}

static void audio_reset()
{
	audio_page(I2C, 0);
	i2c_write_reg(I2C, I2C_ADDR, 0x01, 1u);
}

static void audio_test()
{
	static const uint8_t data[] = {
		0x00, 0x00,		// Page 0
		0x04, 0x03,		// MCLK => PLL => CODEC
		0x05, 0x91,		// PLL enabled, P = 1, R = 1
		0x06, 0x05,		// PLL J = 5
		0x07, 1200u >> 8u,	// PLL D = .1200
		0x08, 1200u & 0xffu,	//
		0x1b, 0x3c,		// I2S, 32 bits, master, no high-z
		//0x1d, 0x20,		// DIN-DOUT loopback
		//0x1d, 0x10,		// ADC-DAC loopback
		0x1d, 0x01,		// No loopback, DAC_MOD_CLK => BCLK
		0x1e, 0x81,		// BCLK N disabled
		0x20, 0x00,		// Using primary interface inputs
		0x21, 0x00,		// Using primary interface outputs
		0x35, 0x12,		// Bus keeper disabled, DOUT from codec
		0x0b, 0x84,		// DAC NDAC = 4
		0x0c, 0x84,		// DAC MDAC = 4
		0x0d, 0,		// DAC DOSR = 32
		0x0e, 32,
		0x12, 0x84,		// ADC NADC = 4
		0x13, 0x84,		// ADC MADC = 4
		0x14, 32,		// ADC DOSR = 32
		0x36, 0x02,		// DIN enabled
		0x3c, 17,		// DAC using PRB_P17
		0x3d, 16,		// ADC using PRB_R16
		0x3f, 0xd4,		// DAC on, data path settings
		0x40, 0x00,		// DAC not muted, independent volume
		0x41, 0xd0,		// DAC left volume = -24dB
		0x42, 0xd0,		// DAC right volume = -24dB
		0x43, 0x80,		// Headset detection enabled
		0x44, 0x0f,		// DRC disabled
		0x47, 0x00,		// Left beep disabled
		0x48, 0x00,		// Right beep disabled
		0x51, 0x80,		// ADC on
		0x52, 0x00,		// ADC not muted, volume fine = 0dB
		0x53, 6 * 2,		// ADC volume coarse = 6dB
		0x56, 0x80,		// AGC enabled, AGC = -5.5dB
		//0x56, 0x00,		// AGC disabled
		//0x57, 0x00,		// AGC settings
		0x58, 33 * 2,		// AGC max = 33dB
		0x74, 0x00,		// DAC volume control pin disabled

		0x00, 0x01,		// Page 1
		0x1f, 0xc4,		// Headphone drivers on, 1.35V
		0x20, 0xc6,		// Speaker amplifiers on
		0x23, 0x44,		// DAC to HP
		0x24, 0x0f,		// HPL analog volume = -7.5dB
		0x25, 0x0f,		// HPR analog volume = -7.5dB
		0x26, 0x00,		// SPL analog volume = 0dB
		0x27, 0x00,		// SPR analog volume = 0dB
		0x28, 0x06,		// HPL driver PGA = 0dB, not muted
		0x29, 0x06,		// HPR driver PGA = 0dB, not muted
		0x2a, 0x14,		// SPL driver PGA = 18dB, not muted
		0x2b, 0x14,		// SPR driver PGA = 18dB, not muted
		0x2c, 0x10,		// DAC high current, HP as headphone
		0x2e, 0x0a,		// MICBIAS force on, MICBIAS = 2.5V
		//0x2f, 0x00,		// MIC PGA 0dB
		0x30, 0x10,		// MIC1RP selected for MIC PGA
		0x31, 0x40,		// CM selected for MIC PGA
	}, *p = data;

	// Write configration sequence
	for (uint32_t i = 0; i != sizeof(data) / sizeof(data[0]) / 2; i++) {
		i2c_write(I2C, I2C_ADDR, p, 2);
		//i2c_write_reg(I2C1, I2C_ADDR, *p, *(p + 1));
		p += 2;
	}
#if 0
	// Read back status
	systick_delay(1000);
	audio_page(I2C, 0);
	static const uint8_t regs[] = {0x05, 0x06, 0x07, 0x08, 0x0b, 0x0c,
				       0x0d, 0x0e, 0x12, 0x13, 0x14,
				       0x24, 0x25, 0x26, 0x27, 0x2c, 0x2e, 0x43,};
	for (uint32_t i = 0; i != sizeof(regs) / sizeof(regs[0]); i++)
		printf("reg 0x%02x: 0x%02x\n", regs[i], (uint8_t)i2c_read_reg(I2C1, I2C_ADDR, regs[i]));
#endif
	// sin(x) lookup table
	int32_t lut[2048];
	for (uint32_t i = 0; i != 2048; i++)
		lut[i] = round(sin(2.0f * M_PI * i / 2048.0) * 0x3000000ul);
	// I2S loopback
	// Receive 2 audio channels
	uint32_t audio[2];
	uint32_t adc[2] = {0, 0};
	for (uint32_t i = 0, j = 0;;) {
		if (SPI1->SR & SPI_SR_TXE_Msk) {
#if 1
			audio[0] = lut[i % 2048];
			audio[1] = lut[i % 2048];
			uint32_t data = audio[!!(SPI1->SR & SPI_SR_CHSIDE_Msk)];
#else
			uint32_t data = adc[!!(SPI1->SR & SPI_SR_CHSIDE_Msk)];
#endif
			if (i++ & 1u)
				SPI1->DR = data & SPI_DR_DR_Msk;
			else
				SPI1->DR = (data >> 16u) & SPI_DR_DR_Msk;
		}
#if 0
		if (SPI2->SR & SPI_SR_RXNE_Msk) {
			uint32_t *p = &adc[!!(SPI2->SR & SPI_SR_CHSIDE_Msk)];
			if (j++ % 2)
				*p |= SPI2->DR & SPI_DR_DR_Msk;
			else
				*p = (SPI2->DR & SPI_DR_DR_Msk) << 16u;
		}
#endif
	}
}

void audio_enable(int enable)
{
	if (enable) {
		// Enable BCLK N, N = 1
		i2c_write_reg(I2C, I2C_ADDR, 0x1e, 0x81);
	} else {
		dbgbkpt();
	}
}

void audio_speaker(int enable)
{
	if (enable) {
		;
	} else {
		;
	}
}
