#include <stm32f7xx.h>
#include <stdint.h>
#include <string.h>
#include <debug.h>
#include <system/systick.h>
#include <peripheral/keyboard.h>
#include <peripheral/led.h>
#include "led_trigger.h"

#define NUM_TRIGGERS	(sizeof(triggers) / sizeof(triggers[0]))
#define NUM_NODES	(sizeof(nodes) / sizeof(nodes[0]))

struct node_t;

static uint32_t frame = 0;

/* Trigger definitions */

struct trigger_t {
	void *data;
	uint8_t channels;
	const uint16_t *(*value)(const struct trigger_t *tp,
				 const struct node_t *np);
};

static const uint16_t *trigger_constant(const struct trigger_t *tp,
					const struct node_t *np);
static const uint16_t *trigger_keyboard(const struct trigger_t *tp,
					const struct node_t *np);
static const uint16_t *trigger_usb(const struct trigger_t *tp,
				   const struct node_t *np);

enum {TGConstant, TGKL, TGKR, TGK1, TGK2, TGK3, TGUSB};
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
};

/* Slave node definitions */

enum config_t {Not = 0x80, Or = 0x40, And = 0x00, Trigger = 0x3f};

struct node_t {
	void *data;
	uint8_t channels;
	uint8_t config[NUM_TRIGGERS];
	void (*set)(struct node_t *p, uint16_t *v);
};

static void node_led_set(struct node_t *np, uint16_t *v);

static struct node_t nodes[] = {
#if LED_NUM != 4
#error Unsupported LED count
#endif
	{(void *)1, 3, {And | TGKL, 0}, node_led_set},
	{(void *)2, 3, {And | TGKR, 0}, node_led_set},
	{(void *)0, 3, {And | TGK1, Or | TGK2, Or | TGUSB, 0}, node_led_set},
	{(void *)3, 3, {And | TGK3, Or | TGK2, Or | TGUSB, 0}, node_led_set},
};

/* Common functions */

static void trigger_calc(struct node_t *np, uint16_t *v)
{
	// Initial value: All 1s
	uint16_t *p = v;
	uint32_t i = np->channels;
	while (i--)
		*p++ = 0xffff;

	uint8_t *cp = np->config;
	for (i = NUM_TRIGGERS; i--; cp++) {
		uint8_t c = *cp;
		uint16_t mask = (c & Not) ? 0xffff : 0x0000;;
		uint32_t min = (c & (And | Or)) == And;
		const struct trigger_t *tp = &triggers[c & Trigger];
		const uint16_t *vp = tp->value(tp, np);
		p = v;
		if (tp->channels == 0) {
			uint16_t v = mask ^ (vp ? 0xffff : 0x0000);
			for (i = np->channels; i--; p++)
				*p = (!min ^ !(*p < v)) ? v : *p;
		} else if (tp->channels < np->channels) {
			// Only take the first channel
			uint16_t v = mask ^ *vp;
			for (i = np->channels; i--; p++)
				*p = (!min ^ !(*p < v)) ? v : *p;
		} else {
			for (i = np->channels; i--; p++) {
				uint16_t v = mask ^ *vp++;
				*p = (!min ^ !(*p < v)) ? v : *p;
			}
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
		uint16_t v[np->channels];
		trigger_calc(np, v);
		np->set(np, v);
	}
	frame++;
}

/* Trigger functions */

static const uint16_t *trigger_constant(const struct trigger_t *tp,
					const struct node_t *np)
{
	static uint16_t trigger_constant_data[NUM_NODES][3] = {
		{0x3ff, 0, 0},
		{0, 0x3ff, 0},
		{0, 0, 0x3ff},
		{0x3ff, 0x3ff, 0},
	};
	return trigger_constant_data[(uint32_t)np->data];
}

static const uint16_t *trigger_keyboard(const struct trigger_t *tp,
					const struct node_t *np)
{
	return (void *)(keyboard_status() & keyboard_masks[(uint32_t)tp->data]);
}

static const uint16_t *trigger_usb(const struct trigger_t *tp,
				   const struct node_t *np)
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

static void node_led_set(struct node_t *np, uint16_t *v)
{
	uint32_t i = (uint32_t)np->data;
	led_set(i, np->channels, v);
}
