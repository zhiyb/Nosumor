#include <module.h>
#include <debug.h>

extern const module_t __module_start__, __module_end__;

#define INDEX(p)    ((p) - &__module_start__)

static void module_msg_deps(uint32_t msgid, void *data)
{
	const uint32_t module_num = INDEX(&__module_end__);
	uint32_t module_state[module_num];
	for (uint32_t i = 0; i != module_num; i++)
		module_state[i] = 0;

	// Always process module init first
	MODULE_MSG_ID(module_init, msgid, data);
	module_state[INDEX(module_init)] = 1;
	if (msgid == HASH("start"))
		dbgprintf(ESC_INFO "[Module] " ESC_DATA "%s" ESC_INFO " started\n", module_init->name);

	// Process other modules
	uint32_t cont;
	do {
		cont = 0;
		for (const module_t *p = &__module_start__; p != &__module_end__; p++) {
			// Skip if module already loaded
			if (module_state[INDEX(p)])
				continue;
			// Check module dependency
			uint32_t skip = 0;
			for (uint32_t *pdep = p->deps; pdep && *pdep; pdep++) {
				const module_t *pd = module_find(*pdep);
				if (!pd) {
					// Dependency not found
					skip = 1;
					dbgbkpt();
					break;
				}
				if (!module_state[INDEX(pd)]) {
					// Dependency not loaded
					skip = 1;
					break;
				}
			}
			if (!skip) {
				// Dependency satisfied, process module
				MODULE_MSG_ID(p, msgid, data);
				module_state[INDEX(p)] = 1;
				if (msgid == HASH("start"))
					dbgprintf(ESC_INFO "[Module] " ESC_DATA "%s" ESC_INFO " started\n", p->name);
				// Continue dependency checking
				cont = 1;
			}
		}
	} while (cont);
}

void module_load()
{
	// Initialisation and start
	module_msg_deps(HASH("init"), 0);
	module_msg_deps(HASH("start"), 0);
}

const module_t *module_find(uint32_t id)
{
	const module_t *p = &__module_start__;
	for (; p->id != id && p != &__module_end__; p++);
	return p == &__module_end__ ? 0 : p;
}

const module_t *module_find_next(const module_t *p)
{
	uint32_t id = p->id;
	while (p++ != &__module_end__)
		if (p->id == id)
			break;
	return p == &__module_end__ ? 0 : p;
}
