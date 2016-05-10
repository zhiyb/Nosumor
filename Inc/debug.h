#ifndef DEBUG_H
#define DEBUG_H

static inline void dbbkpt() {asm ("bkpt #0");}

int dbcmd(int cmd, void *data);

// SYS_WRITE0
static inline void dbputs(char *str) {dbcmd(0x04, str);}

// SYS_WRITEC
static inline void dbputc(char c) {dbcmd(0x03, &c);}

void dbsystem(char *cmd);

#endif // DEBUG_H
