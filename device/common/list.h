#ifndef LIST_H
#define LIST_H

#include <macros.h>

// Reference a link time array
#define LIST(array, type) \
	extern type __start_list_ ## array; \
	extern type __stop_list_ ## array; \
	static type __reserve_list_ ## array[] SECTION("list_" #array) USED = {}
// Add an item to the array
// Unnamed:	LIST_ITEM(array, type) = object;
// Named:	LIST_ITEM(array, type, name) = object;
#define LIST_ITEM_UNNAMED(array, type) \
	static type CONCAT(__item_list_ ## array ## _, __LINE__) SECTION("list_" #array) USED
#define LIST_ITEM_NAMED(array, type, name) \
	static type name SECTION("list_" #array) USED
#define LIST_ITEM_MACRO(_1, _2, _3, macro, ...) macro
#define LIST_ITEM(...)	LIST_ITEM_MACRO(__VA_ARGS__, LIST_ITEM_NAMED, LIST_ITEM_UNNAMED)(__VA_ARGS__)

// List size
#define LIST_SIZE(array) \
	(&__stop_list_ ## array - &__start_list_ ## array)
// Loop iteration
#define LIST_ITERATE(array, type, ptr) \
	for (type *(ptr) = &__start_list_ ## array; (ptr) != &__stop_list_ ## array; (ptr)++)
// Access an item by index
#define LIST_AT(array, i) \
	(*(&__start_list_ ## array + (i)))
// Index of an item
#define LIST_INDEX(array, p) \
	(&(p) - &__start_list_ ## array)

#endif // LIST_H
