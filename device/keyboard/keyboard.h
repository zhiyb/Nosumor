#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <list.h>

// Keyboard event callback handler
typedef void (* const keyboard_callback_t)(uint32_t status);

#define KEYBOARD_CALLBACK(func)	LIST_ITEM(keyboard, keyboard_callback_t) = (func)

#endif // KEYBOARD_H
