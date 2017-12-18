#include <stdexcept>
#include <dev_defs.h>
#include "plugin.h"

#define TIMEOUT_MS	5000

using namespace std;

void Plugin::readReport(hid_device *dev, void *p)
{
	int ret = hid_read_timeout(dev, (uint8_t *)p, VENDOR_REPORT_SIZE, TIMEOUT_MS);
	if (ret == 0)
		throw runtime_error("Report IN timed out");
	else if (ret != VENDOR_REPORT_SIZE) {
		if (ret > 0)
			throw runtime_error("Incorrect report size: " + ret);
		else
			throw runtime_error("Report IN error: " + ret);
	}
}
