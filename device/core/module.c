#include <module.h>
#include <debug.h>

LIST(module, module_t);

#define INDEX(p)	LIST_INDEX(module, *p)
#define SIZE()		LIST_SIZE(module)
#define ITERATE(ptr)	LIST_ITERATE(module, const module_t *ptr, ptr)

static void module_msg_all(uint32_t msgid, void *data)
{
	const uint32_t module_num = SIZE();
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
		ITERATE(p) {
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
	module_msg_all(HASH("init"), 0);
	module_msg_all(HASH("start"), 0);
	module_msg_all(HASH("active"), 0);
}

const module_t *module_find(uint32_t id)
{
	ITERATE(p)
		if (p->id == id)
			return p;
	return 0;
}

const module_t *module_find_next(const module_t *p)
{
	uint32_t id = p->id;
	while (p++ != __stop_list_module)
		if (p->id == id)
			return p;
	return 0;
}
