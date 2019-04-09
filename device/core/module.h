#ifndef MODULE_H
#define MODULE_H

#include <stdint.h>
#include <defines.h>

typedef struct {
	uint32_t id;
	uint32_t *deps;
#ifdef DEBUG
	const char *name;
#endif
	void *inst;
	void *(*h)(void *inst, uint32_t msg, void *data);
} module_t;

extern const module_t *module_init;

void module_load();
const module_t *module_find(uint32_t id);
const module_t *module_find_next(const module_t *p);

// Module ID specific hash
#define MODULE_PREFIX	"__module."
#define MODULE_ID(name)	HASH(MODULE_PREFIX name)

// Create unique module instance
#ifdef DEBUG
#define MODULE(name, deps, param, handler) \
	static const USED SECTION(".module") module_t CONCAT(__module__, __LINE__) = { \
		MODULE_ID(name), (deps), (name), (void *)(param), (handler) \
	}
#else
#define MODULE(name, deps, param, handler) \
	static const USED SECTION(".module") module_t CONCAT(__module__, __LINE__) = { \
		MODULE_ID(name), (deps), (void *)(param), (handler) \
	}
#endif

#define MODULE_MSG_ID(p, msgid, data) \
	(((module_t *)(p))->h(((module_t *)(p))->inst, (msgid), (void *)(data)))
#define MODULE_MSG(p, msg, data)	MODULE_MSG_ID((p), HASH(msg), (data))

#define MODULE_FIND_ID(id) \
	module_find(id)
#define MODULE_FIND(name)		MODULE_FIND_ID(MODULE_ID(name))
#define MODULE_FIND_NEXT(p) \
	module_find_next(p)

#endif // MODULE_H
