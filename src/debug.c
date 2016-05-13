#include <string.h>

int dbcmd(int cmd, void *data)
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

void dbsystem(char *cmd)
{
  int data[2] = {(int)cmd, strlen(cmd)};
  dbcmd(0x12, data);  // SYS_SYSTEM
}
