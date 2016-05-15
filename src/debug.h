#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#define dbbkpt()	asm ("bkpt #0")

static inline int dbcmd(int cmd, void *data)
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
static inline void dbputs(char *str) {dbcmd(0x04, str);}

// SYS_WRITEC
static inline void dbputc(char c) {dbcmd(0x03, &c);}

static inline void dbsystem(char *cmd)
{
	int data[2] = {(int)cmd, strlen(cmd)};
	dbcmd(0x12, data);  // SYS_SYSTEM
}

#ifdef __cplusplus
}
#endif

#endif // DEBUG_H
