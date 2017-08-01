#include <iostream>
#include <stdint.h>
#include <hidapi.h>
#include <dev_defs.h>
#include "logger.h"

using namespace std;

std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("console");

int refreshDeviceList()
{
	int num = 0;
	hid_device_info *devs = hid_enumerate(USB_VID, USB_PID);
	for (hid_device_info *info = devs; info; info = info->next) {
		if (info->usage_page != HID_USAGE_PAGE || info->usage != HID_USAGE)
			continue;
		logger->info(L"Nosumor {}: {}", info->serial_number, info->path);
		num++;
		hid_device *dev = hid_open_path(info->path);
		uint8_t data[7] = {2u, 0x12, 0x34, 0x56, 0x78};
		hid_write(dev, data, 7u);
		data[2] = 0x48;
		hid_write(dev, data, 7u);
		hid_close(dev);
	}
	hid_free_enumeration(devs);
	return num;
}

int main()
{
	logger->set_pattern("[%T %t/%l]: %v");
	logger->set_level(spdlog::level::debug);

	int ret;
	if ((ret = hid_init()) != 0) {
		logger->error("Error initialising hidapi: {}", ret);
		return 1;
	}
	int num = refreshDeviceList();
	logger->info("{} devices found", num);
	hid_exit();
	return 0;
}
