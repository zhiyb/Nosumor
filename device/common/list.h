#ifndef LIST_H
#define LIST_H

#include <macros.h>

// Note, gcc assumes symbols in different sections always have different addresses,
// so we can't use address comparison for checking end of array or array length.
// An array termination item is used for this purpose.

// Create a link time array
#define LIST(name, type, term) \
	static type __list_start_ ## name[] SECTION(".list." #name ".0.start") USED = {}; \
	static type __list_end_ ## name[] SECTION(".list." #name ".9.end") USED = {(term)}
// Reference a link time array
#define LIST_EXTERN(name, type) \
	static type __list_start_ ## name[] SECTION(".list." #name ".0.start") USED = {}
// Add an item to the array
#define LIST_ITEM(name, type) \
	static type CONCAT(__list_item_ ## name ## _, __LINE__) SECTION(".list." #name ".5.item") USED

// Loop iteration
#define LIST_ITERATE(name, type, ptr, term) \
	for (type *(ptr) = __list_start_ ## name; *(ptr) != (term); (ptr)++)
// Access an item by index
#define LIST_AT(name, i) \
	(*(__list_start_ ## name + (i)))
// Index of an item
#define LIST_INDEX(name, p) \
	(&(p) - __list_start_ ## name)

#endif // LIST_H
