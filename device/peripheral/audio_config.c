#include <usb/audio2/usb_audio2_entities.h>
#include <usb/audio2/usb_audio2_desc.h>
#include <usb/audio2/usb_audio2_defs.h>
#include "audio.h"
#include "i2c.h"

#define I2C		AUDIO_I2C
#define I2C_ADDR	AUDIO_I2C_ADDR

// Entities
enum {
	IT_USB = 1,
	CS_PLL,
	CX_In,
	FU_DAC,
	FU_Speaker,
	OT_Speaker,
	OT_Headphones,
};

// Configurations
typedef struct {
	int update;
	struct {
		int8_t vol;	// 0: 0x41, 0x42
	} ch[2];
	uint8_t dac;		// 0: 0x40
	struct {
		uint8_t gain;	// 1: 0x2a, 0x2b
	} sp[2];
} cfg_t;

volatile cfg_t cfg;

// Initial configuration
void audio_init_config()
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

	cfg.ch[0].vol = 0x00;	// DAC left volume = 0dB
	cfg.ch[1].vol = 0x00;	// DAC right volume = 0dB
	cfg.sp[0].gain = 0x14;	// SPL driver PGA = 6dB, not muted
	cfg.sp[1].gain = 0x14;	// SPR driver PGA = 6dB, not muted
	cfg.dac = 0x0c;		// DAC muted, independent volume
	cfg.update = 1;

	// Write configration sequence
	for (uint32_t i = 0; i != sizeof(cmd) / sizeof(cmd[0]) / 2; i++) {
		i2c_write(I2C, I2C_ADDR, p, 2);
		p += 2;
	}
}

// Upload configuration to codec
void audio_config_update()
{
	// Copy configurations to local storage
	if (!cfg.update)
		return;
	__disable_irq();
	cfg_t c = cfg;
	cfg.update = 0;
	__enable_irq();

	// Commands
	const uint8_t cmd[] = {
		0x00, 0x00,		// Page 0
		0x40, c.dac,		// DAC configuration
		0x41, c.ch[0].vol,	// DAC left volume
		0x42, c.ch[1].vol,	// DAC right volume
		0x00, 0x01,		// Page 1
		0x2a, c.sp[0].gain,	// SPL driver PGA = 6dB, not muted
		0x2b, c.sp[1].gain,	// SPR driver PGA = 6dB, not muted
	}, *p = cmd;

	// Write configration sequence
	for (uint32_t i = 0; i != sizeof(cmd) / sizeof(cmd[0]) / 2; i++) {
		i2c_write(I2C, I2C_ADDR, p, 2);
		p += 2;
	}
}

// Enable/disable codec
void audio_config_enable(int enable)
{
	cfg.dac = enable ? 0x00 : 0x0c;
	cfg.update = 1;
}

/* Speaker */

int audio_sp_mute(uint32_t ch)
{
	return !(cfg.sp[ch].gain & 0x04);
}

void audio_sp_mute_set_async(uint32_t ch, int v)
{
	if (v)
		cfg.sp[ch].gain &= ~0x04;
	else
		cfg.sp[ch].gain |= 0x04;
	cfg.update = 1;
}

int32_t audio_sp_vol(uint32_t ch)
{
	int32_t v = (cfg.sp[ch].gain >> 3u) & 0x03u;
	v *= (int)(6 * 256);	// res
	v += (int)(6 * 256);	// min
	return v;
}

void audio_sp_vol_set_async(uint32_t ch, int32_t v)
{
	v -= (int)(6 * 256);	// min
	v /= (int)(6 * 256);	// res
	cfg.sp[ch].gain = (v << 3u) | 0x04;
	cfg.update = 1;
}

/* DAC */

int audio_ch_mute(uint32_t ch)
{
	return !!(cfg.dac & (0x04 << ch));
}

void audio_ch_mute_set_async(uint32_t ch, int v)
{
	if (v)
		cfg.dac |= (0x04 << ch);
	else
		cfg.dac &= ~(0x04 << ch);
	cfg.update = 1;
}

int32_t audio_ch_vol(uint32_t ch)
{
	return (int)(0.5 * 256) * cfg.ch[ch].vol;	// res
}

void audio_ch_vol_set_async(uint32_t ch, int32_t v)
{
	cfg.ch[ch].vol = v / (int)(0.5 * 256);		// res
	cfg.update = 1;
}

// Clock source control
layout1_cur_t cs_valid(usb_audio_t *audio, const uint8_t id)
{
	return id == CS_PLL;
}

layout3_cur_t cs_sam_freq(usb_audio_t *audio, const uint8_t id)
{
	return 192000;
}

int cs_sam_freq_set(usb_audio_t *audio, const uint8_t id, const layout3_cur_t v)
{
	return id == CS_PLL && v == 192000;
}

uint32_t cs_sam_freq_range(usb_audio_t *audio, const uint8_t id, layout3_range_t *buf)
{
	static const layout3_range_t cs_pll[] = {
		{192000ul, 192000ul, 0ul},
	};

	if (id != CS_PLL) {
		dbgbkpt();
		return 0;
	}
	memcpy(buf, cs_pll, sizeof(cs_pll));
	return ASIZE(cs_pll);
}

// Clock selector control
layout1_cur_t cx_selector(usb_audio_t *audio, const uint8_t id)
{
	return id == CX_In ? 1u : 0u;
}

// Feature unit control
layout1_cur_t fu_mute(usb_audio_t *audio, const uint8_t id, const int cn)
{
	if (cn == 0) {
		dbgbkpt();
		return 0;
	}
	switch (id) {
	case FU_DAC:
		return audio_ch_mute(cn - 1u);
	case FU_Speaker:
		return audio_sp_mute(cn - 1u);
	}
	dbgbkpt();
	return 0;
}

int fu_mute_set(usb_audio_t *audio, const uint8_t id, const int cn, layout1_cur_t v)
{
	if (cn == 0) {
		dbgbkpt();
		return 0;
	}
	switch (id) {
	case FU_DAC:
		audio_ch_mute_set_async(cn - 1u, v);
		return 1;
	case FU_Speaker:
		audio_sp_mute_set_async(cn - 1u, v);
		return 1;
	}
	dbgbkpt();
	return 0;
}

layout2_cur_t fu_volume(usb_audio_t *audio, const uint8_t id, const int cn)
{
	if (cn == 0) {
		dbgbkpt();
		return 0;
	}
	switch (id) {
	case FU_DAC:
		return audio_ch_vol(cn - 1u);
	case FU_Speaker:
		return audio_sp_vol(cn - 1u);
	}
	dbgbkpt();
	return 0;
}

int fu_volume_set(usb_audio_t *audio, const uint8_t id, const int cn, const layout2_cur_t v)
{
	if (cn == 0) {
		dbgbkpt();
		return 0;
	}
	switch (id) {
	case FU_DAC:
		audio_ch_vol_set_async(cn - 1u, v);
		return 1;
	case FU_Speaker:
		audio_sp_vol_set_async(cn - 1u, v);
		return 1;
	}
	dbgbkpt();
	return 0;
}

uint32_t fu_volume_range(usb_audio_t *audio, const uint8_t id, const int cn, layout2_range_t *buf)
{
	static const layout2_range_t fu_dac[] = {
		// DAC gain
		{(int)(-63.5 * 256), (int)(24 * 256), (int)(0.5 * 256)},
	};
	static const layout2_range_t fu_speaker[] = {
		// DAC gain
		{(int)(6 * 256), (int)(24 * 256), (int)(6 * 256)},
	};

	if (cn == 0) {
		dbgbkpt();
		return 0;
	}
	switch (id) {
	case FU_DAC:
		memcpy(buf, fu_dac, sizeof(fu_dac));
		return ASIZE(fu_dac);
	case FU_Speaker:
		memcpy(buf, fu_speaker, sizeof(fu_speaker));
		return ASIZE(fu_speaker);
	}
	dbgbkpt();
	return 0;
}

// USB control register
void audio_usb_config(usb_audio_t *audio)
{
	// Clock source
	static const audio_cs_t cs_pll = {
		.valid = &cs_valid,
		.sam_freq = &cs_sam_freq,
		.sam_freq_set = &cs_sam_freq_set,
		.sam_freq_range = &cs_sam_freq_range,
	};
	usb_audio2_register_cs(audio, CS_PLL, &cs_pll, CS_PROGRAMMABLE,
			       CTRL(CS_SAM_FREQ_CONTROL, CTRL_RW) |
			       CTRL(CS_CLOCK_VALID_CONTROL, CTRL_RO),
			       0u, 0u);

	// Clock selector
	static const audio_cx_t cx_in = {
		.selector = &cx_selector,
	};
	const uint8_t cx_in_pins[] = {CS_PLL};
	usb_audio2_register_cx(audio, CX_In, &cx_in, 1u, cx_in_pins,
			       CTRL(CX_CLOCK_SELECTOR_CONTROL, CTRL_RO), 0u);

	// Input terminal
	static const audio_it_t it_usb;
	usb_audio2_register_it(audio, IT_USB, &it_usb, USB_streaming, 0u, CX_In,
			       2u, SP_FL | SP_FR, 0u, 0u, 0u);

	// Feature unit
	static const audio_fu_t fu = {
		.channels = AUDIO_CHANNELS,
		.mute = &fu_mute,
		.mute_set = &fu_mute_set,
		.volume = &fu_volume,
		.volume_set = &fu_volume_set,
		.volume_range = &fu_volume_range,
	};
	const uint32_t fu_ctrls[] = {0u,	// No master channel controls
		CTRL(FU_MUTE_CONTROL, CTRL_RW) | CTRL(FU_VOLUME_CONTROL, CTRL_RW),
		CTRL(FU_MUTE_CONTROL, CTRL_RW) | CTRL(FU_VOLUME_CONTROL, CTRL_RW),
	};
	usb_audio2_register_fu(audio, FU_DAC, &fu, IT_USB, fu_ctrls, 0u);
	usb_audio2_register_fu(audio, FU_Speaker, &fu, FU_DAC, fu_ctrls, 0u);

	// Output terminal
	static const audio_ot_t ot_speaker;
	usb_audio2_register_ot(audio, OT_Speaker, &ot_speaker, Speaker, 0u, FU_Speaker, CX_In, 0u, 0u);
	static const audio_ot_t ot_hp;
	usb_audio2_register_ot(audio, OT_Headphones, &ot_hp, Headphones, 0u, FU_DAC, CX_In, 0u, 0u);
}
