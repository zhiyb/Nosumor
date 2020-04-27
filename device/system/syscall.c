/* Includes */
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>

#include <device.h>
#include <debug.h>
#include <system/system.h>
#include <system/systick.h>

/* Variables */
#undef errno
extern int errno;

register char *stack_ptr asm("sp");

char *__env[1] = { 0 };
char **environ = __env;

extern char __heap_start__;
extern char __heap_end__;

static char *heap_end = &__heap_start__;

/* Functions */
int _getpid(void)
{
	return 1;
}

int _kill(int pid, int sig)
{
	errno = EINVAL;
	return -1;
}

void _exit(int status)
{
	_kill(status, -1);
	for (;;);
}

int _read(int file, char *ptr, int len)
{
	return fio_read(file, ptr, len);
}

int _write(int file, char *ptr, int len)
{
	return fio_write(file, ptr, len);
}

caddr_t _sbrk(int incr)
{
	//extern char end asm("end");
	char *prev_heap_end;

	if (heap_end == 0)
		heap_end = &__heap_start__;

	prev_heap_end = heap_end;
	if (heap_end + incr > &__heap_end__ /*stack_ptr*/) {
		asm("bkpt 0");
		errno = ENOMEM;
		return (caddr_t) -1;
	}

	heap_end += incr;

	return (caddr_t) prev_heap_end;
}

size_t heap_size()
{
	return heap_end - &__heap_start__;
}

size_t heap_max_size()
{
	return &__heap_end__ - &__heap_start__;
}

#if DEBUG >= 5
static void heap_debug()
{
	static size_t psize = 0;
	size_t hsize = heap_size();
	if (psize == hsize)
		return;
	psize = hsize;
	dbgprintf(ESC_DEBUG "%lu\tcore: HEAP RAM allocated %u bytes\n", systick_cnt(), hsize);
}

IDLE_HANDLER(&heap_debug);
#endif

int _close(int file)
{
	return -1;
}


int _fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file)
{
	return 1;
}

int _lseek(int file, int ptr, int dir)
{
	return 0;
}

int _open(char *path, int flags, ...)
{
	/* Pretend like we always fail */
	return -1;
}

int _wait(int *status)
{
	errno = ECHILD;
	return -1;
}

int _unlink(char *name)
{
	errno = ENOENT;
	return -1;
}

int _times(struct tms *buf)
{
	return -1;
}

int _stat(char *file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _link(char *old, char *new)
{
	errno = EMLINK;
	return -1;
}

int _fork(void)
{
	errno = EAGAIN;
	return -1;
}

int _execve(char *name, char **argv, char **env)
{
	errno = ENOMEM;
	return -1;
}

int __io_putchar(int ch)
{
	return fio_write(STDOUT_FILENO, (void *)&ch, 1);
}

int __io_getchar(void)
{
	int ch, res;
	res = fio_read(STDIN_FILENO, (void *)&ch, 1);
	return res != 1 ? res : ch;
}
