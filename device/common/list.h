#ifndef LIST_H
#define LIST_H

#include <macros.h>

// Reference a link time array
#define LIST(name, type) \
	extern type __start_list_ ## name; \
	extern type __stop_list_ ## name; \
	static type __reserve_list_ ## name[] SECTION("list_" #name) USED = {}
// Add an item to the array
#define LIST_ITEM(name, type) \
	static type CONCAT(__item_list_ ## name ## _, __LINE__) SECTION("list_" #name) USED

// List size
#define LIST_SIZE(name) \
	(&__stop_list_ ## name - &__start_list_ ## name)
// Loop iteration
#define LIST_ITERATE(name, type, ptr) \
	for (type *(ptr) = &__start_list_ ## name; (ptr) != &__stop_list_ ## name; (ptr)++)
// Access an item by index
#define LIST_AT(name, i) \
	(*(__start_list_ ## name + (i)))
// Index of an item
#define LIST_INDEX(name, p) \
	(&(p) - __start_list_ ## name)

#endif // LIST_H
