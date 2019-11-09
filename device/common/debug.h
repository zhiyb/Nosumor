#ifndef DEBUG_H
#define DEBUG_H

#include <device.h>

#define dbgexist()	(CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)

#if DEBUG

#include <stdio.h>
#include <escape.h>
#include <macros.h>

#define VARIANT	"DEBUG-" STRING(DEBUG)

extern void flushCache();

#define dbgprintf	printf
#define dbgbkpt()	do { \
	flushCache(); \
	__BKPT(0); \
} while (0)
#define panic()		do { \
	printf("\nPanicked at %s:%u: %s()\n", __FILE__, __LINE__, __PRETTY_FUNCTION__); \
	dbgbkpt(); \
} while (1)

static inline int dbgcmd(int cmd, void *data)
{
	register int r0 asm ("r0");
	asm ("mov r0, %0\n\t"
	     "mov r1, %1\n\t"
	     "bkpt #0xAB\n\t"
	     :
	     : "r" (cmd), "r" (data)
	     : "r0", "r1");
	return r0;
}

// SYS_WRITE0
static inline void dbgputs(char *str) {dbgcmd(0x04, str);}

// SYS_WRITEC
static inline void dbgputc(char c) {dbgcmd(0x03, &c);}

#if 0
static inline void dbgsystem(char *cmd)
{
	int data[2] = {(int)cmd, (int)strlen(cmd)};
	dbgcmd(0x12, data);  // SYS_SYSTEM
}
#endif

#else	// DEBUG

#define VARIANT	"RELEASE"

#define dbgprintf(...)	((void)0)
#define dbgbkpt()	((void)0)
#define panic()		do { \
	printf("\nPanicked  at %d: %s()\n", __LINE__, __PRETTY_FUNCTION__); \
	flushCache(); \
	NVIC_SystemReset(); \
} while (1)

#define dbgputs(str)	((void)0)
#define dbgputc(str)	((void)0)
#define dbgsystem(str)	((void)0)

#endif	// DEBUG

#endif // DEBUG_H
