#include <stdexcept>
#include <dev_defs.h>
#include "plugin.h"

#define TIMEOUT_MS	5000

using namespace std;

void Plugin::send(hid_device *dev, uint8_t channel, api_report_t *rp)
{
	rp->id = HID_REPORT_ID;
	rp->size += API_BASE_SIZE;
	rp->cksum = 0;
	rp->channel = channel;

	// Checksum
	uint8_t c = 0, *p = rp->raw;
	for (uint8_t i = rp->size; i--;)
		c += *p++;
	rp->cksum = -c;

	hid_write(dev, rp->raw, API_REPORT_SIZE);
}

void Plugin::recv(hid_device *dev, api_report_t *rp)
{
	int ret = hid_read_timeout(dev, (uint8_t *)rp, API_REPORT_SIZE, TIMEOUT_MS);
	if (ret == 0)
		throw runtime_error("Report IN timed out");
	else if (ret != API_REPORT_SIZE) {
		if (ret > 0)
			throw runtime_error("Incorrect report size: " + to_string(ret));
		else
			throw runtime_error("Report IN error: " + to_string(ret));
	}

	// Check payload size
	if (rp->size < API_BASE_SIZE || rp->size > API_REPORT_SIZE)
		throw runtime_error("Invalid report size: " + to_string(rp->size));

	// Checksum
	uint8_t c = 0, *p = rp->raw;
	for (uint8_t i = rp->size; i--;)
		c += *p++;
	if (c != 0)
		throw runtime_error("Checksum failed");

	rp->size -= API_BASE_SIZE;
}
