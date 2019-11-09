#ifndef SYSTEM_H
#define SYSTEM_H

int fio_write(int file, char *ptr, int len);
int fio_read(int file, char *ptr, int len);

// Flush everything, including stdout
void flushCache();

// Debug related functions that might take a while to finish
void system_debug_process();

#endif // SYSTEM_H
