#ifndef DEBUG_H
#define DEBUG_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define dbgexist()	(CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)

#ifdef DEBUG

#define VARIANT	"DEBUG"
#define dbgbkpt()	asm ("bkpt #0")

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
#define dbgbkpt()	((void)0)
#define dbgputs(str)	((void)0)
#define dbgputc(str)	((void)0)
#define dbgsystem(str)	((void)0)

#endif	// DEBUG

#ifdef __cplusplus
}
#endif

#endif // DEBUG_H
