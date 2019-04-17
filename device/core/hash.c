#include <defines.h>
#include <debug.h>

#define MODULE_PREFIX		"_module."
#define USB_EP_MODULE_PREFIX	"_usb_ep_module."

#define C(name)		{HASH(#name), #name}
#define C_M(name)	{HASH(MODULE_PREFIX #name), MODULE_PREFIX #name}
#define C_UEM(name)	{HASH(USB_EP_MODULE_PREFIX #name), USB_EP_MODULE_PREFIX #name}

#ifdef DEBUG

static const struct {
	uint32_t v;
	const char *s;
} hash[] = {
	C(init), C(start), C(active), C(config), C(stdio), C(info), C(get), C(set),
	C(status), C(connect), C(disconnect),
	C(tick.get), C(tick.delay),
	C(IN), C(OUT),
	C_M(init), C_M(uart), C_M(led), C_M(keyboard), C_M(usb.core),
	C_UEM(usb.core), C_UEM(usb.keyboard), C_UEM(usb.mouse), C_UEM(usb.joystick),
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
