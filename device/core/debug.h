#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <escape.h>
#include <device.h>

#define dbgexist()	(CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)

#ifdef DEBUG

#include <string.h>
#include <escape.h>

#define VARIANT	"DEBUG"

extern void flushCache();

#define dbgprintf	printf
#define dbgbkpt()	do { \
	if (dbgexist()) { \
		fflush(stdout); \
		flushCache(); \
		__BKPT(0); \
	} \
} while (0)
#define panic()		do { \
	printf("\nPanicked at %s:%d: %s()\n", __FILE__, __LINE__, __PRETTY_FUNCTION__); \
	fflush(stdout); \
	flushCache(); \
	__BKPT(0); \
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

// SYS_SYSTEM
static inline void dbgsystem(char *cmd)
{
	int data[2] = {(int)cmd, (int)strlen(cmd)};
	dbgcmd(0x12, data);
}

#else	// DEBUG

#define VARIANT	"RELEASE"

#define dbgprintf(...)	((void)0)
#define dbgbkpt()	((void)0)
#define panic()		do { \
	fflush(stdout); \
	__BKPT(0); \
} while (1)

#define dbgputs(str)	((void)0)
#define dbgputc(str)	((void)0)
#define dbgsystem(str)	((void)0)

#endif	// DEBUG

#endif // DEBUG_H
