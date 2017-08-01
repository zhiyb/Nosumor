#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <stdint.h>
#include <libgen.h>
#include <hidapi.h>
#include <dev_defs.h>
#include "logger.h"

using namespace std;

std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("console");

void flash(hid_device *dev, char *file)
{
	logger->info("Flashing {}...", file);
	vendor_report_t report;
	report.id = HID_REPORT_ID;
	// Reset flashing buffer
	report.size = VENDOR_REPORT_BASE_SIZE;
	report.type = FlashReset;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	// Write hex file content
	ifstream ifs(file);
	string str;
	while (ifs >> str) {
		if (str.at(0) != ':')
			throw runtime_error("Invalid Intel HEX format");
		report.size = VENDOR_REPORT_BASE_SIZE;
		report.type = FlashData;
		uint8_t *p = report.payload;
		for (unsigned int i = 1; i < str.length(); i += 2, report.size++) {
			int v = stoul(str.substr(i, 2), 0, 16);
			*p++ = v;
		}
		hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	}
	// Check hex validity
	report.size = VENDOR_REPORT_BASE_SIZE;
	report.type = FlashCheck;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	if (hid_read(dev, report.raw, VENDOR_REPORT_SIZE) != VENDOR_REPORT_SIZE)
		throw runtime_error("Incorrect FlashCheck report size");
	if (!report.payload[0])
		throw runtime_error("Received invalid HEX content");
	// Start flashing
	report.size = VENDOR_REPORT_BASE_SIZE;
	report.type = FlashStart;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
}

void process(hid_device *dev, int argc, char **argv)
{
	hid_set_nonblocking(dev, 0);
	// Process arguments
	++argv, --argc;
	if (strcmp(*argv, "flash") == 0) {
		if (argv++, !--argc)
			return;
		flash(dev, *argv);
	}
}

int main(int argc, char **argv)
{
	logger->set_pattern("[%T %t/%l]: %v");
	logger->set_level(spdlog::level::debug);

	if (argc <= 1) {
		clog << "Usage: " << basename(argv[0]) << " operation [arguments]" << endl << endl;
		clog << "Available operations:" << endl;
		clog << "  flash program.hex  | Flash program.hex to target devices" << endl;
		return 1;
	}

	int ret;
	if ((ret = hid_init()) != 0) {
		logger->error("Error initialising hidapi: {}", ret);
		return 1;
	}

	int num = 0;
	hid_device_info *devs = hid_enumerate(USB_VID, USB_PID);
	for (hid_device_info *info = devs; info; info = info->next) {
		if (info->usage_page != HID_USAGE_PAGE || info->usage != HID_USAGE)
			continue;
		auto mf = info->manufacturer_string ?: L"(null)";
		auto prod = info->product_string ?: L"(null)";
		auto sn = info->serial_number ?: L"(null)";
		logger->info(L"({}) {} {}: {}", mf, prod, sn, info->path);
		num++;
		hid_device *dev = hid_open_path(info->path);
		if (!dev) {
			logger->error("Cannot open HID device");
			continue;
		}
		try {
			process(dev, argc, argv);
		} catch (exception &e) {
			logger->error("Error: {}", e.what());
		}
		hid_close(dev);
	}
	hid_free_enumeration(devs);
	logger->info("{} devices processed", num);

	hid_exit();
	return 0;
}
