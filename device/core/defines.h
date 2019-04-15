#ifndef DEFINES_H
#define DEFINES_H

#include <stdint.h>
#include <string.h>

// Static assertion
// http://www.pixelbeat.org/programming/gcc/static_assert.html
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
/* These can't be used after statements in c89. */
#ifdef __COUNTER__
#define STATIC_ASSERT(e,m) \
    ;enum { ASSERT_CONCAT(static_assert_, __COUNTER__) = 1/(int)(!!(e)) }
#else
/* This can't be used twice on the same line so ensure if using in headers
   * that the headers are not included twice (by wrapping in #ifndef...#endif)
   * Note it doesn't cause an issue when used on same line of separate modules
   * compiled with gcc -combine -fwhole-program.  */
#define STATIC_ASSERT(e,m) \
    ;enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(int)(!!(e)) }
#endif

// Static string hash
// http://lolengine.net/blog/2011/12/20/cpp-constant-string-hash
#define STR_H1(s,i,x)   (x*65599u+(const uint8_t)s[(i)<strlen(s)?strlen(s)-1-(i):strlen(s)])
#define STR_H4(s,i,x)   STR_H1(s,i,STR_H1(s,i+1,STR_H1(s,i+2,STR_H1(s,i+3,x))))
#define STR_H16(s,i,x)  STR_H4(s,i,STR_H4(s,i+4,STR_H4(s,i+8,STR_H4(s,i+12,x))))
#define STR_H64(s,i,x)  STR_H16(s,i,STR_H16(s,i+16,STR_H16(s,i+32,STR_H16(s,i+48,x))))
#define STR_H256(s,i,x) STR_H64(s,i,STR_H64(s,i+64,STR_H64(s,i+128,STR_H64(s,i+192,x))))
// Limit: 256 characters
#define HASH(s)         ((const uint32_t)(STR_H256(s,0,0)^(STR_H256(s,0,0)>>16)))

#ifdef DEBUG
uint32_t hash_check();
#endif

// Compiler attributes
#define PACKED          __attribute__((packed))
#define USED            __attribute__((used))
#define ALIGN(b)        __attribute__((aligned(b)))
#define ALIGNED         __attribute__((aligned))
#define SECTION(s)      __attribute__((section(s)))
#define STATIC_INLINE   __attribute__((always_inline)) static inline

// Miscellaneous fixed point math function
#define ASIZE(a)	(sizeof((a)) / sizeof((a)[0]))
#define ROUND(a, b)	(((a) + ((b) / 2)) / (b))
#define CEIL(a, b)	(((a) + (b) - 1) / (b))
#define FLOOR(a, b)	((a) / (b))
#define MIN(a, b)	((a) < (b) ? (a) : (b))

#define UNUSED(x)       (void)(x)

#define _CONCAT(a, b)   a ## b
#define CONCAT(a, b)    _CONCAT(a, b)
#define _STRING(x)	#x
#define STRING(x)	_STRING(x)

#endif // DEFINES_H
