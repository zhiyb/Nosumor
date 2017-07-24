#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void audio_init();
void audio_out_enable(int enable);

// Audio device information, unit: 1/256 dB
static inline int32_t audio_vol_min() {return -63.5 * 256;}
static inline int32_t audio_vol_max() {return 24 * 256;}
static inline int32_t audio_vol_res() {return 0.5 * 256;}

#ifdef __cplusplus
}
#endif

#endif // AUDIO_H
