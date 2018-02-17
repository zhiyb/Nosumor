#ifndef VENDOR_H
#define VENDOR_H

#include <api_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

struct api_reg_t {
	void (* const handler)(void *hid, uint8_t channel,
			       void *data, uint8_t size, void *payload);
	uint16_t version;
	const char name[];
};

void api_register(const struct api_reg_t *reg);
void api_send(void *hid, uint8_t channel, uint8_t size);

void api_init(void *hid);
void api_process(void *hid, api_report_t *rp);

// 25 bytes space (null-terminated) required
void uid_str(char *s);

#ifdef __cplusplus
}
#endif

#endif // VENDOR_H
