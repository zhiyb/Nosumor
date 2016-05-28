#ifndef USB_DEBUG_H
#define USB_DEBUG_H

//#undef DEBUG

#ifdef DEBUG

#include "usart1.h"
#define writeChar(c)		usart1WriteChar((c))
#define writeString(str)	usart1WriteString((str))
#define dumpHex(v)		usart1DumpHex((v))

#else

#define writeChar(c)		((void)0)
#define writeString(str)	((void)0)
#define dumpHex(v)		((void)0)

#endif

#include "debug.h"

#endif // USB_DEBUG_H
