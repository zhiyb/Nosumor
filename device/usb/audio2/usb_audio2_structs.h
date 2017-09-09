#ifndef USB_AUDIO2_STRUCTS_H
#define USB_AUDIO2_STRUCTS_H

#include <string.h>
#include <debug.h>
#include <macros.h>
#include <peripheral/audio.h>

#define DATA_MAX_SIZE	(AUDIO_BUFFER_SIZE >> 1ul)
#define CHANNELS	(1u + AUDIO_CHANNELS)

#ifdef __cplusplus
extern "C" {
#endif

// Parameter block layout types

typedef uint8_t layout1_cur_t;

typedef struct PACKED {
	uint8_t min, max, res;
} layout1_range_t;

typedef uint16_t layout2_cur_t;

typedef struct PACKED {
	uint16_t min, max, res;
} layout2_range_t;

typedef uint32_t layout3_cur_t;

typedef struct PACKED {
	uint32_t min, max, res;
} layout3_range_t;

// Private data types

typedef struct data_t {
	struct {
		layout3_cur_t freq;
		struct PACKED {
			layout3_range_t range[1];
		};
	} cs;
	struct {
		layout2_cur_t vol[CHANNELS];
		struct PACKED {
			layout2_range_t range[CHANNELS];
		};
		layout1_cur_t mute[CHANNELS];
	} fu;
	struct PACKED {
		union {
			struct {
				uint16_t wNumSubRanges;
				uint8_t range[];
			};
			uint8_t raw[48];
		};
	} buf;
	int ep_data, ep_feedback;
} data_t;

// Parameter block operations

static inline uint16_t layout_cur_get(data_t *data, int type, void *p)
{
	const unsigned int msize = 1u << (type - 1u);
	memcpy(data->buf.raw, p, msize);
	return msize;
}

static inline uint32_t layout_cur(int type, void *p)
{
	uint32_t v = 0;
	const unsigned int msize = 1u << (type - 1u);
	memcpy(&v, p, msize);
	dbgprintf("%lx|", v);
	return v;
}

static inline uint16_t layout_range_get(data_t *data, int type, void *p, uint16_t cnt)
{
	const unsigned int msize = 1u << (type - 1u);
	const uint16_t size = 3u * msize * cnt;
	data->buf.wNumSubRanges = cnt;
	memcpy(data->buf.range, p, size);
	return 2u + size;
}

#ifdef __cplusplus
}
#endif

#endif // USB_AUDIO2_STRUCTS_H
