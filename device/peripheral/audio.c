#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <stm32f7xx.h>
#include "../macros.h"
#include "../escape.h"
#include "../debug.h"
#include "../systick.h"
#include "i2c.h"
#include "audio.h"

#define I2C		I2C1
#define I2C_ADDR	0b0011000u

#define STREAM_TX	DMA2_Stream5
#define STREAM_RX	DMA1_Stream3

typedef struct {
	struct {
		int8_t vol;	// 0: 0x41, 0x42
	} ch[2];
	uint8_t dac;		// 0: 0x40
	struct {
		uint8_t gain;	// 1: 0x2a, 0x2b
	} sp[2];
} cfg_t;

static struct {
	// Audio buffer
	uint8_t buf[AUDIO_BUFFER_LENGTH][AUDIO_CHANNELS][AUDIO_SAMPLE_SIZE] ALIGN(AUDIO_FRAME_SIZE);
	// Buffer write data pointer
	void * volatile ptr;
	// Data counter
	uint32_t cnt_transfer, cnt_data;
	int32_t offset;
	// Configurations
	volatile cfg_t cfg;
	volatile int update;
} data;

static void audio_reset();
static void audio_config();
static void audio_tick(uint32_t tick);

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
	i2c_init(I2C);

	if (!i2c_check(I2C, I2C_ADDR)) {
		dbgbkpt();
		return;
	}

	// Software reset
	audio_reset();
	// Reset audio buffer
	memset(&data, 0, sizeof(data));
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
	SPI1->CR2 = SPI_CR2_TXDMAEN_Msk;
	SPI1->I2SCFGR = SPI_I2SCFGR_I2SMOD_Msk | SPI_I2SCFGR_CHLEN_Msk |
			(0b00u << SPI_I2SCFGR_I2SCFG_Pos) |
			(0b10u << SPI_I2SCFGR_DATLEN_Pos);
	// I2S2: Slave receive, 32-bit
	SPI2->CR1 = 0;
	SPI2->CR2 = 0;
	SPI2->I2SCFGR = SPI_I2SCFGR_I2SMOD_Msk | SPI_I2SCFGR_CHLEN_Msk |
			(0b01u << SPI_I2SCFGR_I2SCFG_Pos) |
			(0b10u << SPI_I2SCFGR_DATLEN_Pos);

	// DMA2: Channel 3: Stream 5 (SPI1_TX)
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN_Msk;
	// Disable stream
	STREAM_TX->CR = 0ul;
	// Memory to peripheral, circular, 32bit -> 16bit, very high priority
	STREAM_TX->CR = (3ul << DMA_SxCR_CHSEL_Pos) | (0b11ul << DMA_SxCR_PL_Pos) |
			(0b11ul << DMA_SxCR_MSIZE_Pos) | (0b01ul << DMA_SxCR_PSIZE_Pos) |
			(0b01ul << DMA_SxCR_DIR_Pos) | DMA_SxCR_MINC_Msk | DMA_SxCR_CIRC_Msk;
	// Peripheral address
	STREAM_TX->PAR = (uint32_t)&SPI1->DR;
	// FIFO control
	STREAM_TX->FCR = DMA_SxFCR_DMDIS_Msk;
	// Memory address
	STREAM_TX->M0AR = (uint32_t)data.buf;
	// Number of data items
	STREAM_TX->NDTR = AUDIO_TRANSFER_SIZE;
	// Enable DMA stream
	STREAM_TX->CR |= DMA_SxCR_EN_Msk;

	// DMA1: Channel 0: Stream 3 (SPI2_RX)
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN_Msk;

	// Enable I2S audio interfaces
	SPI1->I2SCFGR |= SPI_I2SCFGR_I2SE_Msk;
	SPI2->I2SCFGR |= SPI_I2SCFGR_I2SE_Msk;

	// Codec GPIO (interrupt): PC4
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN_Msk;
	GPIO_MODER(GPIOC, 4, 0b00);	// 00: Input mode
	GPIO_PUPDR(GPIOC, 4, GPIO_PUPDR_UP);

	// Start
	audio_config();
	systick_register_handler(audio_tick);
}

static void audio_tick(uint32_t tick)
{
	static uint32_t prev = AUDIO_TRANSFER_SIZE;
	__disable_irq();
	// Data transferred
	uint16_t ndtr = STREAM_TX->NDTR;
	// Size increment: (prev - NDTR + size) % size
	uint32_t n = (prev - ndtr) & (AUDIO_TRANSFER_SIZE - 1ul);
	prev = ndtr;
	data.cnt_transfer += n;
	if (data.ptr)
		data.offset -= AUDIO_TRANSFER_BYTES(n);
	__enable_irq();

	// Clear buffer if lost sync
	if (data.offset < -(int32_t)AUDIO_BUFFER_SIZE) {
		memset(data.buf, 0, sizeof(data.buf));
		data.offset = 0;
	}
}

void audio_process()
{
	// Copy configurations to local storage
	if (!data.update)
		return;
	__disable_irq();
	cfg_t cfg = data.cfg;
	data.update = 0;
	__enable_irq();

	// Commands
	const uint8_t cmd[] = {
		0x00, 0x00,		// Page 0
		0x40, cfg.dac,		// DAC configuration
		0x41, cfg.ch[0].vol,	// DAC left volume
		0x42, cfg.ch[1].vol,	// DAC right volume
		0x00, 0x01,		// Page 1
		0x2a, cfg.sp[0].gain,	// SPL driver PGA = 6dB, not muted
		0x2b, cfg.sp[1].gain,	// SPR driver PGA = 6dB, not muted
	}, *p = cmd;

	// Write configration sequence
	for (uint32_t i = 0; i != sizeof(cmd) / sizeof(cmd[0]) / 2; i++) {
		i2c_write(I2C, I2C_ADDR, p, 2);
		p += 2;
	}
}

uint32_t audio_transfer_cnt()
{
	return data.cnt_transfer;
}

uint32_t audio_data_cnt()
{
	return data.cnt_data;
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

static void audio_config()
{
	static const uint8_t cmd[] = {
		0x00, 0x00,		// Page 0
		0x04, 0x03,		// MCLK => PLL => CODEC
		0x05, 0x91,		// PLL enabled, P = 1, R = 1
		0x06, 0x05,		// PLL J = 5
		0x07, 1200u >> 8u,	// PLL D = .1200
		0x08, 1200u & 0xffu,	//
		0x1b, 0x3c,		// I2S, 32 bits, master, no high-z
		//0x1d, 0x20,		// DIN-DOUT loopback
		//0x1d, 0x10,		// ADC-DAC loopback
		0x1d, 0x00,		// No loopback, DAC_CLK => BCLK
		0x1e, 0x82,		// Enable BCLK N, N = 2
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
		0x40, 0x0c,		// DAC muted, independent volume
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
		0x2a, 0x04,		// SPL driver PGA = 6dB, not muted
		0x2b, 0x04,		// SPR driver PGA = 6dB, not muted
		0x2c, 0x10,		// DAC high current, HP as headphone
		0x2e, 0x0a,		// MICBIAS force on, MICBIAS = 2.5V
		//0x2f, 0x00,		// MIC PGA 0dB
		0x30, 0x10,		// MIC1RP selected for MIC PGA
		0x31, 0x40,		// CM selected fvoidor MIC PGA
	}, *p = cmd;

	data.cfg.ch[0].vol = 0x00;	// DAC left volume = 0dB
	data.cfg.ch[1].vol = 0x00;	// DAC right volume = 0dB
	data.cfg.sp[0].gain = 0x14;	// SPL driver PGA = 6dB, not muted
	data.cfg.sp[1].gain = 0x14;	// SPR driver PGA = 6dB, not muted
	data.cfg.dac = 0x0c;		// DAC muted, independent volume
	data.update = 1;
	audio_process();

	// Write configration sequence
	for (uint32_t i = 0; i != sizeof(cmd) / sizeof(cmd[0]) / 2; i++) {
		i2c_write(I2C, I2C_ADDR, p, 2);
		p += 2;
	}
}

void audio_out_enable(int enable)
{
	data.cfg.dac = enable ? 0x00 : 0x0c;
	data.update = 1;
	data.offset = 0;
	data.ptr = 0;
	if (enable)
		dbgprintf(ESC_BLUE "Audio unmuted\n");
	else
		dbgprintf(ESC_BLUE "Audio muted\n");
}

static inline uint32_t *next_frame()
{
	// ((buffer - remaining) + buffering) & (alignment & buffer)
	uint32_t mem = (AUDIO_BUFFER_SIZE) - AUDIO_TRANSFER_BYTES(STREAM_TX->NDTR)
			+ (AUDIO_FRAME_SIZE << 4u);
	mem &= ~(AUDIO_FRAME_SIZE - 1ul) & (AUDIO_BUFFER_SIZE - 1ul);
	return (uint32_t *)((void *)data.buf + mem);
}

void audio_play(void *p, uint32_t size)
{
	uint32_t *mem = data.ptr ?: next_frame(), *ptr = (uint32_t *)p;
	// Update offset
	audio_tick(0);
	__disable_irq();
	data.offset += size;
	if (!data.ptr) {	// ((mem - buf) - (buffer - remaining) + buffer) % buffer
		data.offset = (void *)mem - (void *)data.buf + AUDIO_TRANSFER_BYTES(STREAM_TX->NDTR);
		data.offset %= AUDIO_BUFFER_SIZE;
	}
	__enable_irq();
	// Fill buffer
	size >>= 2u;
	data.cnt_data += size >> 1u;
	if (size & 1u)
		dbgbkpt();
	while (size--) {
		*mem++ = ((*ptr) << 16ul) | ((*ptr) >> 16ul);
		ptr++;
		if ((void *)mem == (void *)data.buf + AUDIO_BUFFER_SIZE)
			mem = (void *)data.buf;
	}
	data.ptr = mem;
}

/* Speaker */

int32_t audio_sp_vol(uint32_t ch)
{
	if (ch >= 2u) {
		dbgbkpt();
		return 0;
	}
	int32_t v = (data.cfg.sp[ch].gain >> 3u) & 0x03u;
	v *= audio_sp_vol_res();
	v += audio_sp_vol_min();
	return v;
}

void audio_sp_vol_set_async(uint32_t ch, int32_t v)
{
	if (ch >= 2u) {
		dbgbkpt();
		return;
	}
	v -= audio_sp_vol_min();
	v /= audio_sp_vol_res();
	data.cfg.sp[ch].gain = (v << 3u) | 0x04;
	data.update = 1;
}

/* DAC */

int32_t audio_ch_vol(uint32_t ch)
{
	if (ch >= 2u) {
		dbgbkpt();
		return 0;
	}
	return data.cfg.ch[ch].vol * audio_ch_vol_res();
}

void audio_ch_vol_set_async(uint32_t ch, int32_t v)
{
	if (ch >= 2u) {
		dbgbkpt();
		return;
	}
	data.cfg.ch[ch].vol = v / audio_ch_vol_res();
	data.update = 1;
}

int32_t audio_buffering()
{
	return data.offset;
}
