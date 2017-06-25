#ifndef FIO_H
#define FIO_H

void fio_setup(void (*put)(void *, char), char (*get)(void *), void *p);

#endif // FIO_H
