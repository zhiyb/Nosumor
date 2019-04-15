#ifndef LIST_H
#define LIST_H

#include <defines.h>

// Create a link time array
#define LIST(name, type) \
	static type __reserve_list_ ## name[] SECTION("list_" #name) USED = {}; \
	extern type __start_list_ ## name[]; \
	extern type __stop_list_ ## name[]
// Loop iteration
#define LIST_ITERATE(name, decl, ptr) \
	for (decl = __start_list_ ## name; (ptr) != __stop_list_ ## name; (ptr)++)
// Index of an item
#define LIST_INDEX(name, p) \
	(&(p) - __start_list_ ## name)
// Array size
#define LIST_SIZE(name) \
	LIST_INDEX(name, *__stop_list_ ## name)

// Add an item to the array
#define LIST_ITEM(name, type) \
	static type CONCAT(_list_ ## name ## _, __LINE__) SECTION("list_" #name) USED

#endif // LIST_H
