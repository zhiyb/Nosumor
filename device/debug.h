#ifndef DEBUG_H
#define DEBUG_H

#include <stm32f7xx.h>
#include <stdio.h>

#define dbgexist()	(CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
#define panic()		do { \
	printf("\nPanic at %s():%d\n", __PRETTY_FUNCTION__, __LINE__); \
	fflush(stdout); \
	__BKPT(0); \
} while (1)

#ifdef DEBUG

#include <string.h>
#include <escape.h>

#define VARIANT	"DEBUG"

#define dbgprintf	printf
#define dbgbkpt()	do { \
	fflush(stdout); \
	__BKPT(0); \
} while (0)

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

static inline void dbgsystem(char *cmd)
{
	int data[2] = {(int)cmd, (int)strlen(cmd)};
	dbgcmd(0x12, data);  // SYS_SYSTEM
}

#else	// DEBUG

#define VARIANT	"RELEASE"

#define dbgprintf(...)	((void)0)
#define dbgbkpt()	((void)0)
#define dbgputs(str)	((void)0)
#define dbgputc(str)	((void)0)
#define dbgsystem(str)	((void)0)

#endif	// DEBUG

#endif // DEBUG_H
