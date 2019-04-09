#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <string.h>
#include "fio.h"

void (*putf)(void *, char c);
char (*getf)(void *);
void *pd;

void fio_setup(void (*put)(void *, char c), char (*get)(void *), void *p)
{
	putf = put;
	getf = get;
	pd = p;
}

int __io_putchar(int ch)
{
	if (ch == '\n')
		__io_putchar('\r');
	putf(pd, ch);
	return ch;
}

int __io_getchar(void)
{
	return getf(pd);
}
