#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <stm32f7xx.h>
#include <macros.h>
#include <escape.h>
#include <debug.h>
#include <system/systick.h>
#include "i2c.h"
#include "audio.h"

#if HWVER >= 0x0100
#define STREAM_TX	DMA2_Stream1
#define STREAM_RX	DMA2_Stream7
#else
#define STREAM_TX	DMA2_Stream5
#define STREAM_RX	DMA1_Stream3
#endif

static struct {
	// Audio buffer
	uint8_t buf[AUDIO_BUFFER_LENGTH][AUDIO_CHANNELS][AUDIO_SAMPLE_SIZE] ALIGN(AUDIO_FRAME_SIZE);
	uint8_t rxbuf[AUDIO_BUFFER_LENGTH][AUDIO_CHANNELS][AUDIO_SAMPLE_SIZE] ALIGN(AUDIO_FRAME_SIZE);
	// Buffer write data pointer
	void * volatile ptr;
	// Data counter
	uint32_t cnt_transfer, cnt_data;
	int32_t offset;
} data SECTION(.dtcm);

void audio_init_reset(void *i2c);
void audio_init_config();
void audio_usb_config(usb_t *usb, usb_audio_t *audio);
void audio_config_update();
void audio_config_enable(int enable);

static void audio_tick(uint32_t tick);

void audio_init(void *i2c, usb_t *usb, usb_audio_t *audio)
{
	if (!i2c_check(i2c, AUDIO_I2C_ADDR))
		return;

	// Software reset
	audio_init_reset(i2c);
	// Reset audio buffer
	memset(&data, 0, sizeof(data));

#if HWVER >= 0x0100
	// Initialise SAI GPIOs
	// SAI1_A (slave transmit): SD(PC1)
	// SAI2_B (slave receive): SD(PA0), SCK(PA2), FS(PA12)
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN_Msk | RCC_AHB1ENR_GPIOCEN_Msk;
	// PC1
	GPIO_MODER(GPIOC, 1, 0b10);	// 10: Alternative function mode
	GPIO_AFRL(GPIOC, 1, 6);		// AF6: SAI1
	GPIO_OSPEEDR(GPIOC, 1, 0b01);	// Medium speed (25MHz)
	GPIO_OTYPER_PP(GPIOC, 1);
	// PA0
	GPIO_MODER(GPIOA, 0, 0b10);
	GPIO_AFRL(GPIOA, 0, 10);	// AF10: SAI2
	// PA2
	GPIO_MODER(GPIOA, 2, 0b10);
	GPIO_AFRL(GPIOA, 2, 8);		// AF8: SAI2
	// PA12
	GPIO_MODER(GPIOA, 12, 0b10);
	GPIO_AFRH(GPIOA, 12, 8);	// AF8: SAI2

	// Initialise SAI modules
	RCC->APB2ENR |= RCC_APB2ENR_SAI1EN_Msk | RCC_APB2ENR_SAI2EN_Msk;

	// SAI2, block B: Slave receive, 32-bit
	// Synchronisation output from block B, no synchronisation inputs
	SAI2->GCR = (0b10 << SAI_GCR_SYNCOUT_Pos);
	// Disable block A
	SAI2_Block_A->CR1 = 0;
	// Mono, async, rising edge sample, MSB first, 32-bit,
	// free protocol, slave receiver
	SAI2_Block_B->CR1 = SAI_xCR1_MONO_Msk | SAI_xCR1_CKSTR_Msk |
			(0b111 << SAI_xCR1_DS_Pos) |
			(0b11 << SAI_xCR1_MODE_Pos);
	// No mute detection, FIFO flush, threshold empty
	SAI2_Block_B->CR2 = SAI_xCR2_FFLUSH_Msk | (0b000 << SAI_xCR2_FTH_Pos);
	// Frame offset, active low, channel identification,
	// active length 32-bit, frame length 64-bit
	SAI2_Block_B->FRCR = SAI_xFRCR_FSOFF_Msk | SAI_xFRCR_FSDEF_Msk |
			(31 << SAI_xFRCR_FSALL_Pos) |
			(63 << SAI_xFRCR_FRL_Pos);
	// Enable slots 1:0, 2 slots, 32-bit, 0 offset
	SAI2_Block_B->SLOTR = (0b11 << SAI_xSLOTR_SLOTEN_Pos) |
			(1 << SAI_xSLOTR_NBSLOT_Pos) |
			(0b10 << SAI_xSLOTR_SLOTSZ_Pos) |
			(0 << SAI_xSLOTR_FBOFF_Pos);
	// Disable interrupts
	SAI2_Block_B->IMR = 0;

	// SAI1, block A: Slave transmit, 32-bit
	// No synchronisation outputs, synchronisation input from SAI2
	SAI1->GCR = (1 << SAI_GCR_SYNCIN_Pos);
	// Disable block B
	SAI1_Block_B->CR1 = 0;
	// Stereo, external sync, failing edge setup, MSB first, 32-bit,
	// free protocol, slave transmitter
	SAI1_Block_A->CR1 = (0b10 << SAI_xCR1_SYNCEN_Pos) |
			SAI_xCR1_CKSTR_Msk | (0b111 << SAI_xCR1_DS_Pos) |
			(0b10 << SAI_xCR1_MODE_Pos);
	// Mute using last value, mute enable, FIFO flush, threshold empty
	SAI1_Block_A->CR2 = SAI_xCR2_MUTEVAL_Msk | SAI_xCR2_MUTE_Msk |
			SAI_xCR2_FFLUSH_Msk | (0b000 << SAI_xCR2_FTH_Pos);
	// Frame offset, active low, channel identification,
	// active length 32-bit, frame length 64-bit
	SAI1_Block_A->FRCR = SAI_xFRCR_FSOFF_Msk | SAI_xFRCR_FSDEF_Msk |
			(31 << SAI_xFRCR_FSALL_Pos) |
			(63 << SAI_xFRCR_FRL_Pos);
	// Enable slots 1:0, 2 slots, 32-bit, 0 offset
	SAI1_Block_A->SLOTR = (0b11 << SAI_xSLOTR_SLOTEN_Pos) |
			(1 << SAI_xSLOTR_NBSLOT_Pos) |
			(0b10 << SAI_xSLOTR_SLOTSZ_Pos) |
			(0 << SAI_xSLOTR_FBOFF_Pos);
	// Disable interrupts
	SAI1_Block_A->IMR = 0;

	// Initialise TX DMA (DMA2, Stream 1, Channel 0)
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN_Msk;
	// Disable stream
	STREAM_TX->CR = 0ul;
	// Memory to peripheral, circular, 32bit -> 32bit, very high priority
	STREAM_TX->CR = (0ul << DMA_SxCR_CHSEL_Pos) |
			(0b11ul << DMA_SxCR_PL_Pos) |
			(0b10ul << DMA_SxCR_MSIZE_Pos) |
			(0b10ul << DMA_SxCR_PSIZE_Pos) |
			(0b01ul << DMA_SxCR_DIR_Pos) |
			DMA_SxCR_MINC_Msk | DMA_SxCR_CIRC_Msk;
	// Peripheral address
	STREAM_TX->PAR = (uint32_t)&SAI1_Block_A->DR;
	// FIFO control
	STREAM_TX->FCR = DMA_SxFCR_DMDIS_Msk;
	// Memory address
	STREAM_TX->M0AR = (uint32_t)data.buf;
	// Number of data items
	STREAM_TX->NDTR = AUDIO_TRANSFER_SIZE;
	// Enable DMA stream
	STREAM_TX->CR |= DMA_SxCR_EN_Msk;

	// Initialise RX DMA (DMA2, Stream 7, Channel 0)
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN_Msk;
	// Disable stream
	STREAM_RX->CR = 0ul;
	// Peripheral to memory, circular, 32bit -> 32bit, very high priority
	STREAM_RX->CR = (0 << DMA_SxCR_CHSEL_Pos) |
			(0b11 << DMA_SxCR_PL_Pos) |
			(0b10 << DMA_SxCR_MSIZE_Pos) |
			(0b10 << DMA_SxCR_PSIZE_Pos) |
			(0b00 << DMA_SxCR_DIR_Pos) |
			DMA_SxCR_MINC_Msk | DMA_SxCR_CIRC_Msk;
	// Peripheral address
	STREAM_RX->PAR = (uint32_t)&SAI2_Block_B->DR;
	// FIFO control
	STREAM_RX->FCR = DMA_SxFCR_DMDIS_Msk;
	// Memory address
	STREAM_RX->M0AR = (uint32_t)data.rxbuf;
	// Number of data items
	STREAM_RX->NDTR = AUDIO_TRANSFER_SIZE;
	// Enable DMA stream
	STREAM_RX->CR |= DMA_SxCR_EN_Msk;

	// Enable audio blocks, enable DMA
	SAI2_Block_B->CR1 |= SAI_xCR1_SAIEN_Msk | SAI_xCR1_DMAEN_Msk;
	SAI1_Block_A->CR1 |= SAI_xCR1_SAIEN_Msk | SAI_xCR1_DMAEN_Msk;
#else
	// Initialise I2S GPIOs
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
			(0b10ul << DMA_SxCR_MSIZE_Pos) | (0b01ul << DMA_SxCR_PSIZE_Pos) |
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
#endif

	// Start
	audio_init_config();
	//systick_register_handler(audio_tick);
	audio_usb_config(usb, audio);
	audio_process();
}

static void audio_tick(uint32_t tick)
{
	static uint32_t prev = AUDIO_TRANSFER_SIZE;
	// Data transferred
	uint16_t ndtr = STREAM_TX->NDTR;
	// Size increment: (prev - NDTR + size) % size
	uint32_t n = (prev - ndtr) & (AUDIO_TRANSFER_SIZE - 1ul);
	prev = ndtr;
	data.cnt_transfer += n;
	if (data.ptr)
		data.offset -= AUDIO_TRANSFER_BYTES(n);
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
	if (enable) {
#if HWVER >= 0x0100
		// Mute disable
		SAI1_Block_A->CR2 &= ~SAI_xCR2_MUTE_Msk;
#endif
		dbgprintf(ESC_MSG "[AUDIO] " ESC_ENABLE "Unmuted\n");
	} else {
#if HWVER >= 0x0100
		// Mute enable
		SAI1_Block_A->CR2 |= SAI_xCR2_MUTE_Msk;
#endif
		dbgprintf(ESC_MSG "[AUDIO] " ESC_DISABLE "Muted\n");
		memset(data.buf, 0, sizeof(data.buf));
	}
}

static inline uint32_t *next_frame(uint32_t nbytes)
{
	// ((buffer - remaining) + buffering) & (alignment & buffer)
	uint32_t mem = AUDIO_BUFFER_SIZE - nbytes + (AUDIO_FRAME_SIZE << 5u);
	mem &= ~(AUDIO_FRAME_SIZE - 1ul) & (AUDIO_BUFFER_SIZE - 1ul);
	return (uint32_t *)((void *)data.buf + mem);
}

void audio_play(void *p, uint32_t size)
{
	uint32_t nbytes = AUDIO_TRANSFER_BYTES(STREAM_TX->NDTR);
#if HWVER >= 0x0100
	uint64_t *mem = data.ptr ?: next_frame(nbytes);
	uint64_t *mptr = mem, *ptr = (uint64_t *)p;
	uint32_t slots = size >> 3u;
#else
	uint32_t *mem = data.ptr ?: next_frame(nbytes);
	uint32_t *mptr = mem, *ptr = (uint32_t *)p;
	uint32_t slots = size >> 2u;
	if (slots & 1u)
		dbgbkpt();
#endif
	while (slots--) {
#if HWVER >= 0x0100
		*mptr++ = *ptr++;
#else
		*mptr++ = __REV16(__REV(*ptr++));
#endif
		// Circular buffer wrap around
		if ((void *)mptr == (void *)data.buf + AUDIO_BUFFER_SIZE)
			mptr = (void *)data.buf;
	}
	// Update offset
	audio_tick(0);
	data.offset += size;
	if (!data.ptr)
		data.offset = 0;
	while (data.offset > (int)AUDIO_BUFFER_SIZE)
		data.offset -= AUDIO_BUFFER_SIZE;
	while (data.offset < -(int)AUDIO_BUFFER_SIZE)
		data.offset += AUDIO_BUFFER_SIZE;
	data.cnt_data += size >> 3u;
	data.ptr = mptr;
}

#ifdef DEBUG

#define TEST_FREQ	620
#define TEST_SIZE	(192000 / TEST_FREQ)
#define TEST_BUF_SIZE	(TEST_SIZE * AUDIO_FRAME_SIZE)

static void *test = 0;

void audio_test()
{
	if (test) {
		// Redirect DMA
		// Disable DMA stream
		STREAM_TX->CR &= ~DMA_SxCR_CIRC_Msk;
		while (STREAM_TX->CR & DMA_SxCR_EN_Msk);
#if HWVER >= 0x0100
		// Clear DMA flags
		DMA2->LIFCR = DMA_LIFCR_CTCIF1_Msk | DMA_LIFCR_CHTIF1_Msk |
				DMA_LIFCR_CTEIF1_Msk | DMA_LIFCR_CDMEIF1_Msk |
				DMA_LIFCR_CFEIF1_Msk;
#else
		// Clear DMA flags
		DMA2->HIFCR = DMA_HIFCR_CTCIF5_Msk | DMA_HIFCR_CHTIF5_Msk |
				DMA_HIFCR_CTEIF5_Msk | DMA_HIFCR_CDMEIF5_Msk |
				DMA_HIFCR_CFEIF5_Msk;
#endif
		// Memory address
		STREAM_TX->M0AR = (uint32_t)data.buf;
		// Number of data items
		STREAM_TX->NDTR = AUDIO_TRANSFER_SIZE;
		// Enable DMA stream
		STREAM_TX->CR |= DMA_SxCR_EN_Msk | DMA_SxCR_CIRC_Msk;

		// Free memory space
		free(test);
		test = 0;
		return;
	}

	// Generate sin wave
	if (!test) {
		test = malloc(TEST_BUF_SIZE);
		if (!test) {
			dbgbkpt();
			return;
		}
		uint32_t *p = test;
		for (uint32_t i = TEST_SIZE; i != 0; i--) {
			uint32_t v = round(sin((float)i / (float)TEST_SIZE) * 0x3fffffff);
#if HWVER < 0x0100
			v = __REV16(__REV(v));
#endif
			*p++ = v;
			*p++ = v;
		}
	}

	// Redirect DMA
	// Disable DMA stream
	STREAM_TX->CR &= ~DMA_SxCR_CIRC_Msk;
	while (STREAM_TX->CR & DMA_SxCR_EN_Msk);
#if HWVER >= 0x0100
	// Clear DMA flags
	DMA2->LIFCR = DMA_LIFCR_CTCIF1_Msk | DMA_LIFCR_CHTIF1_Msk |
			DMA_LIFCR_CTEIF1_Msk | DMA_LIFCR_CDMEIF1_Msk |
			DMA_LIFCR_CFEIF1_Msk;
#else
	// Clear DMA flags
	DMA2->HIFCR = DMA_HIFCR_CTCIF5_Msk | DMA_HIFCR_CHTIF5_Msk |
			DMA_HIFCR_CTEIF5_Msk | DMA_HIFCR_CDMEIF5_Msk |
			DMA_HIFCR_CFEIF5_Msk;
#endif
	// Memory address
	STREAM_TX->M0AR = (uint32_t)test;
	// Number of data items
	STREAM_TX->NDTR = AUDIO_TRANSFER_CNT(TEST_BUF_SIZE);
	// Enable DMA stream
	STREAM_TX->CR |= DMA_SxCR_EN_Msk | DMA_SxCR_CIRC_Msk;

	// Enable codec
	audio_out_enable(1);
	audio_process();
}

#endif	// DEBUG
