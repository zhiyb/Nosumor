#ifndef SYSTEM_H
#define SYSTEM_H

int fio_write(int file, char *ptr, int len);
int fio_read(int file, char *ptr, int len);

void flushCache();

#endif // SYSTEM_H
