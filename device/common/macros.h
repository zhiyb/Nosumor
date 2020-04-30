#ifndef MACROS_H
#define MACROS_H

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

#define FUNC(f)		if (f) f

#endif // MACROS_H
