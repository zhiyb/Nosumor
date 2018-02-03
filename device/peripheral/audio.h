#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_I2C_ADDR	0b0011000u

// Audio buffer information
#define AUDIO_CHANNELS		2ul
#define AUDIO_SAMPLE_SIZE	4ul
#define AUDIO_FRAME_SIZE	(AUDIO_CHANNELS * AUDIO_SAMPLE_SIZE)
#define AUDIO_BUFFER_LENGTH	64ul
#define AUDIO_BUFFER_SIZE	(AUDIO_FRAME_SIZE * AUDIO_BUFFER_LENGTH)
#if HWVER >= 0x0100
#define AUDIO_TRANSFER_CNT(n)	((n) >> 2u)
#define AUDIO_TRANSFER_BYTES(n)	((n) << 2u)
#else
#define AUDIO_TRANSFER_CNT(n)	((n) >> 1u)
#define AUDIO_TRANSFER_BYTES(n)	((n) << 1u)
#endif
#define AUDIO_TRANSFER_SIZE	(AUDIO_TRANSFER_CNT(AUDIO_BUFFER_SIZE))
#define AUDIO_FRAME_TRANSFER	(AUDIO_TRANSFER_CNT(AUDIO_FRAME_SIZE))

typedef struct usb_t usb_t;
typedef struct usb_audio_t usb_audio_t;

void audio_init(void *i2c, usb_t *usb, usb_audio_t *audio);
void audio_out_enable(int enable);
void audio_play(void *p, uint32_t size);
void audio_process();
uint32_t audio_transfer_cnt();
uint32_t audio_data_cnt();
int32_t audio_buffering();

#ifdef DEBUG
void audio_test();
#endif

#ifdef __cplusplus
}
#endif

#endif // AUDIO_H
