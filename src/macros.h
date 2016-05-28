/*
 * Author: Yubo Zhi (normanzyb@gmail.com)
 */

#ifndef MACROS_H
#define MACROS_H

#include <stdint.h>

#define _NOP() __asm__ __volatile__("nop")

#define CONCAT(a,b)	a ## b
#define CONCAT_E(a,b)	CONCAT(a, b)

#define STRINGIFY(x)	#x
#define TOSTRING(x)	STRINGIFY(x)

#define ARRAY_SIZE(a)	(sizeof((a)) / sizeof((a)[0]))

#define H8(i)		(((i) >> 8U) & 0xff)
#define L8(i)		((i) & 0xff)
#define DIV8(i)		((i) >> 3U)
#define MOD8(i)		((i) & 0x07)
#define HL(reg, i)	(*((uint32_t *)&(reg) + DIV8(i)))
#define BV(i)		(1UL << (i))

// Combine RGB components
#define COLOUR_888(r, g, b)	((((uint32_t)(r) & 0xff) << 16) | \
				(((uint32_t)(g) & 0xff) << 8) | \
				(((uint32_t)(b) & 0xff) << 0))
#define COLOUR_565(r, g, b)	((((uint16_t)(r) & 0xf8) << 8) | \
				(((uint16_t)(g) & 0xfc) << 3) | \
				(((uint16_t)(b) & 0xf8) >> 3))
// Convert between RGB-888 and RGB-565
#define COLOUR_565_888(c)	((((uint32_t)(c) & 0x00f80000) >> 8) | \
				(((uint32_t)(c) & 0x0000fc00) >> 5) | \
				(((uint32_t)(c) & 0x000000f8) >> 3))
// TODO #define COLOUR_888_565(c)	()
// Invert RGB & BGR
#define COLOUR_888_I(c)	((((uint32_t)(c) & 0x00ff0000) >> 16) | \
			((uint32_t)(c) & 0x0000ff00) | \
			(((uint32_t)(c) & 0x000000ff) << 16))
#define COLOUR_565_I(c)	(((((uint16_t)(c) & 0xf800) >> 11) | \
			((uint16_t)(c) & 0x07e0) | \
			(((uint16_t)(c) & 0x001f) << 11))
// Extract RGB components
#define RED_888(c)	(((uint32_t)(c) >> 16) & 0xff)
#define GREEN_888(c)	(((uint32_t)(c) >> 8) & 0xff)
#define BLUE_888(c)	(((uint32_t)(c) >> 0) & 0xff)
#define RED_565(c)	(((uint16_t)(c) >> 8) & 0xff)
#define GREEN_565(c)	(((uint16_t)(c) >> 3) & 0xff)
#define BLUE_565(c)	(((uint16_t)(c) << 3) & 0xff)

#endif
