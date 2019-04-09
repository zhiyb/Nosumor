#include <defines.h>
#include <module.h>
#include <debug.h>

#define C(name)		{HASH(name), name}
#define CM(name)	{MODULE_ID(name), MODULE_PREFIX name}

#ifdef DEBUG

static const struct {
	uint32_t v;
	const char *s;
} hash[] = {
	C("init"), C("start"), C("config"), C("stdio"), C("info"), C("status"),
	C("tick.handler.install"), C("tick.get"), C("tick.delay"),
	C("handler.install"),
	CM("init"), CM("uart"), CM("led"), CM("keyboard"),
};

// For checking string hash collision
uint32_t hash_check()
{
	uint32_t fail = 0;
	for (uint32_t i = 0; i != ASIZE(hash); i++) {
		printf(ESC_DEBUG "[Hash] 0x%08lx  %s\n", hash[i].v, hash[i].s);
		for (uint32_t j = i + 1; j != ASIZE(hash); j++) {
			if (hash[i].v == hash[j].v) {
				fail = 1;
				dbgbkpt();
			}
		}
	}
	return fail;
}

#endif
