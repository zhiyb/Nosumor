#ifndef USB_DEBUG_H
#define USB_DEBUG_H

#include <stdio.h>
#include "../escape.h"
#include "../debug.h"

#undef DEBUG

#ifdef DEBUG
#define dbgprintf	printf
#else	// DEBUG
#define dbgprintf(...)
#endif	// DEBUG

#endif // USB_DEBUG_H
