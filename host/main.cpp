#include <iostream>
#include <stdint.h>
#include <hidapi.h>

using namespace std;

int refreshDeviceList()
{
	int num = 0;
	hid_device_info *devs = hid_enumerate(USB_VID, USB_PID);
	for (hid_device_info *info = devs; info; info = info->next) {
		if (info->usage_page != HID_USAGE_PAGE || info->usage != HID_USAGE)
			continue;
		clog << "Nosumor SN " << info->serial_number << ": " << info->path << endl;
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
	int ret;
	if ((ret = hid_init()) != 0) {
		cerr << "Error initialising hidapi: " << ret << endl;
		return 1;
	}
	int num = refreshDeviceList();
	clog << num << " devices found" << endl;
	hid_exit();
	return 0;
}
