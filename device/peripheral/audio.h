#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void audio_init();
void audio_out_enable(int enable);
void audio_play(void *p, uint32_t size);
uint32_t audio_transfer_cnt();

// Audio buffer information
#define AUDIO_CHANNELS		2ul
#define AUDIO_SAMPLE_SIZE	4ul
#define AUDIO_FRAME_SIZE	(AUDIO_CHANNELS * AUDIO_SAMPLE_SIZE)
#define AUDIO_BUFFER_LENGTH	256ul
#define AUDIO_BUFFER_SIZE	(AUDIO_FRAME_SIZE * AUDIO_BUFFER_LENGTH)
#define AUDIO_TRANSFER_CNT(n)	((n) >> 1u)
#define AUDIO_TRANSFER_SIZE	(AUDIO_TRANSFER_CNT(AUDIO_BUFFER_SIZE))
#define AUDIO_FRAME_TRANSFER	(AUDIO_TRANSFER_CNT(AUDIO_FRAME_SIZE))

// Audio device information, unit: 1/256 dB
static inline int32_t audio_vol_min() {return -63.5 * 256;}
static inline int32_t audio_vol_max() {return 24 * 256;}
static inline int32_t audio_vol_res() {return 0.5 * 256;}

#ifdef __cplusplus
}
#endif

#endif // AUDIO_H
