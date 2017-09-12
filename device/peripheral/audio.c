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

#define I2C		AUDIO_I2C
#define I2C_ADDR	AUDIO_I2C_ADDR

#define STREAM_TX	DMA2_Stream5
#define STREAM_RX	DMA1_Stream3

static struct {
	// Audio buffer
	uint8_t buf[AUDIO_BUFFER_LENGTH][AUDIO_CHANNELS][AUDIO_SAMPLE_SIZE] ALIGN(AUDIO_FRAME_SIZE);
	// Buffer write data pointer
	void * volatile ptr;
	// Data counter
	uint32_t cnt_transfer, cnt_data;
	int32_t offset;
} data;

void audio_init_config();
void audio_usb_config(usb_audio_t *audio);
void audio_config_update();
void audio_config_enable(int enable);

static void audio_tick(uint32_t tick);

void audio_init(usb_audio_t *audio)
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
	i2c_write_reg(I2C, I2C_ADDR, 0x00, 0x00);	// Page 0
	i2c_write_reg(I2C, I2C_ADDR, 0x01, 1u);
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
	audio_init_config();
	systick_register_handler(audio_tick);
	audio_usb_config(audio);
	audio_process();
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
	audio_config_update();
}

uint32_t audio_transfer_cnt()
{
	return data.cnt_transfer;
}

uint32_t audio_data_cnt()
{
	return data.cnt_data;
}

int32_t audio_buffering()
{
	return data.offset;
}

void audio_out_enable(int enable)
{
	audio_config_enable(enable);
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
