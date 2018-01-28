#include <stm32f7xx.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <debug.h>
#include <system/systick.h>
#include <peripheral/keyboard.h>
#include <peripheral/led.h>
#include "led_trigger.h"

#define CHANNELS	4

#define NUM_TRIGGERS	(sizeof(triggers) / sizeof(triggers[0]))
#define NUM_CHANNELS	(sizeof(nodes[0].config) / sizeof(nodes[0].config[0]))
#define NUM_NODES	(sizeof(nodes) / sizeof(nodes[0]))

struct node_t;

static uint32_t frame = 0;

/* Trigger definitions */

struct trigger_t {
	void *data;
	uint8_t channels;
	const float *(*value)(const struct trigger_t *tp,
				 const struct node_t *np, int ch);
};

static const float *trigger_constant(const struct trigger_t *tp,
					const struct node_t *np, int ch);
static const float *trigger_keyboard(const struct trigger_t *tp,
					const struct node_t *np, int ch);
static const float *trigger_breath(const struct trigger_t *tp,
				     const struct node_t *np, int ch);
static const float *trigger_usb(const struct trigger_t *tp,
				   const struct node_t *np, int ch);

enum {TGConstant, TGKL, TGKR, TGK1, TGK2, TGK3, TGUSB, TGBreath};
static const struct trigger_t triggers[] = {
#if KEYBOARD_KEYS != 5
#error Unsupported keyboard key count
#endif
	{(void *)0, 3, trigger_constant},
	{(void *)0, 0, trigger_keyboard},
	{(void *)1, 0, trigger_keyboard},
	{(void *)2, 0, trigger_keyboard},
	{(void *)3, 0, trigger_keyboard},
	{(void *)4, 0, trigger_keyboard},
	{(void *)0, 0, trigger_usb},
	{(void *)0, 1, trigger_breath},
};

/* Slave node definitions */

enum config_t {Inv = 0x80, Min = 0x40, Max = 0x00, Trigger = 0x3f};

struct node_t {
	void *data;
	uint8_t channels;
	uint8_t config[CHANNELS][NUM_TRIGGERS];
	void (*set)(struct node_t *p, float *v);
};

static void node_led_set(struct node_t *np, float *v);

static struct node_t nodes[] = {
#if LED_NUM != 4
#error Unsupported LED count
#endif
	{(void *)1, 3, {{TGKL, Min | 0}, {TGBreath, Min | 0}}, node_led_set},
	{(void *)2, 3, {{TGKR, Min | 0}, {TGBreath, Min | 0}}, node_led_set},
	{(void *)0, 3, {{TGK1, TGK2, Min | 0},
			{TGBreath, Min | 0}, {TGUSB, Min | 0}}, node_led_set},
	{(void *)3, 3, {{TGK3, TGK2, Min | 0},
			{TGBreath, Min | 0}, {TGUSB, Min | 0}}, node_led_set},
};

/* Common functions */

static float mix(float a, float b, uint8_t c)
{
	if (c & Inv)
		b = 1.0 - b;
	return (((c & (Min | Max)) == Max) ^ (b < a)) ? b : a;
}

static void calc(struct node_t *np, int ch, float *v)
{
	// Initial values
	float *p = v;
	uint8_t *cp = np->config[ch];
	uint32_t i = np->channels;
	float c = ((*cp & (Min | Max)) == Max) ? 0.0 : 1.0;
	for (; i--; *p++ = c);

	for (i = NUM_TRIGGERS; i--; cp++) {
		uint8_t c = *cp;
		const struct trigger_t *tp = &triggers[c & Trigger];
		const float *vp = tp->value(tp, np, ch);
		p = v;
		if (tp->channels == 0) {
			float v = vp ? 1.0 : 0.0;
			for (i = np->channels; i--; p++)
				*p = mix(*p, v, c);
		} else if (tp->channels < np->channels) {
			// Only take the first channel
			for (i = np->channels; i--; p++)
				*p = mix(*p, *vp, c);
		} else {
			for (i = np->channels; i--; p++)
				*p = mix(*p, *vp++, c);
		}
		// Finish if reached trigger 0 (constants)
		if ((c & Trigger) == 0)
			break;
	}
}

void led_trigger_process()
{
	uint32_t n = NUM_NODES;
	while (n--) {
		struct node_t *np = &nodes[n];
		float v[np->channels];
		calc(np, 0, v);
		uint32_t ch;
		for (ch = 1; ch != NUM_CHANNELS; ch++) {
			float vt[np->channels];
			calc(np, ch, vt);
			float *vp = v, *vtp = vt;
			uint8_t c = np->config[ch][0] & ~Inv;
			uint32_t i;
			for (i = np->channels; i--; vp++, vtp++)
				*vp = mix(*vp, *vtp, c);
		}
		np->set(np, v);
	}
	frame++;
}

/* Trigger functions */

static float trigger_constant_data[NUM_CHANNELS][NUM_NODES][3] = {
	{{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},
	{{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 1.0, 0.0}},
	{{0.0, 0.0, 0.1}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.1}},
};

static const float *trigger_constant(const struct trigger_t *tp,
					const struct node_t *np, int ch)
{
	return trigger_constant_data[ch][(uint32_t)np->data];
}

static const float *trigger_keyboard(const struct trigger_t *tp,
					const struct node_t *np, int ch)
{
	return (void *)(keyboard_status() & keyboard_masks[(uint32_t)tp->data]);
}

static const float *trigger_breath(const struct trigger_t *tp,
				     const struct node_t *np, int ch)
{
	// http://sean.voisen.org/blog/2011/10/breathing-led-with-arduino/
	static uint32_t tick = 0, fn = -1;
	static float level = 0;
	// Update on frame updates
	if (fn != frame) {
		fn = frame;
		// Update every 1 ms
		if (tick != systick_cnt())
			// (exp(sin(x) + 1) - 1) / (exp(2) - 1)
			level = (exp(sin((float)systick_cnt() * M_PI / 2000.0)
				     + 1.0) - 1.0) / (exp(2.0) - 1.0);
	}
	return &level;
}

static const float *trigger_usb(const struct trigger_t *tp,
				   const struct node_t *np, int ch)
{
	static uint32_t tick = 0, fn = -1, active = 0;
	extern usb_t usb;
	// Update on frame updates
	if (fn != frame) {
		fn = frame;
		// Update every 16 ms
		if ((tick ^ systick_cnt()) & ~15ul) {
			tick = systick_cnt();
			active = usb_active(&usb);
		}
	}
	return (void *)active;
}

/* Slave node functions */

static void node_led_set(struct node_t *np, float *v)
{
	uint16_t c[np->channels], *p = c;
	uint32_t i = np->channels;
	while (i--)
		*p++ = *v++ * 1023;
	led_set((uint32_t)np->data, np->channels, c);
}
