#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <stdint.h>
#ifdef __GNUC__
#include <libgen.h>
#endif
#include <hidapi.h>
#include <dev_defs.h>
#include <api.h>
#include "logger.h"

#define TIMEOUT_MS	5000

using namespace std;

std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("console");

void send_report(hid_device *dev, api_report_t *rp)
{
	rp->id = HID_REPORT_ID;
	rp->size += API_BASE_SIZE;
	rp->cksum = 0;
	logger->debug("Report OUT: size {}, channel {:02x}",
		     rp->size, rp->channel);

	// Checksum
	uint8_t c = 0, *p = rp->raw;
	for (uint8_t i = rp->size; i--;)
		c += *p++;
	rp->cksum = -c;

	hid_write(dev, rp->raw, API_REPORT_SIZE);
}

void recv_report(hid_device *dev, api_report_t *rp)
{
	int ret = hid_read_timeout(dev, (uint8_t *)rp, API_REPORT_SIZE, TIMEOUT_MS);
	if (ret == 0)
		throw runtime_error("Report IN timed out");
	else if (ret != API_REPORT_SIZE) {
		if (ret > 0)
			throw runtime_error("Incorrect report size: " + ret);
		else
			throw runtime_error("Report IN error: " + ret);
	}

	// Check payload size
	if (rp->size < API_BASE_SIZE || rp->size > API_REPORT_SIZE)
		throw runtime_error("Invalid report size: " + rp->size);

	// Checksum
	uint8_t c = 0, *p = rp->raw;
	for (uint8_t i = rp->size; i--;)
		c += *p++;
	if (c != 0)
		throw runtime_error("Checksum failed");

	logger->debug("Report IN: size {}, channel {:02x}",
		     rp->size, rp->channel);
}

#if 0
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
	read_report(dev, report.raw);
	if (!report.payload[0])
		throw runtime_error("Received invalid HEX content");
	// Start flashing
	report.size = VENDOR_REPORT_BASE_SIZE;
	report.type = FlashStart;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
}
#endif

#if 0
void reset(hid_device *dev)
{
	logger->info("Resetting device...");
	vendor_report_t report;
	report.id = HID_REPORT_ID;
	report.size = VENDOR_REPORT_BASE_SIZE;
	report.type = FlashReset;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	report.type = FlashStart;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
}
#endif

static void ping(hid_device *dev, uint8_t channel)
{
	api_report_t report;
	report.size = 0u;
	report.channel = channel;
	send_report(dev, &report);

	recv_report(dev, &report);
	api_pong_t *p = (api_pong_t *)report.payload;
	logger->info("Hardware version: {:04x}, software version: {:04x}",
		     p->hw_ver, p->sw_ver);
	logger->info("Device UID: {:08x}{:08x}{:08x}",
		     p->uid[2], p->uid[1], p->uid[0]);
	logger->info("Flash size: {} KiB", p->fsize);
}

static void keycode_update(hid_device *dev, uint8_t channel,
			   uint8_t btn, uint8_t code)
{
	api_report_t report;
	report.size = sizeof(api_keycode_t);
	report.channel = channel;

	api_keycode_t *p = (api_keycode_t *)report.payload;
	p->btn = btn;
	p->code = code;

	send_report(dev, &report);
	logger->info("Set keycode of button {} to {}", btn, code);
}

static uint8_t channels(hid_device *dev)
{
	api_report_t report;
	report.size = 0u;
	send_report(dev, &report);

	recv_report(dev, &report);
	return ((api_info_t *)report.payload)->channel;
}

static string channel_info(hid_device *dev, uint8_t channel,
			   uint16_t *version)
{
	api_report_t report;
	report.size = 1u;
	report.channel = 0u;

	api_info_t *p = (api_info_t *)report.payload;
	p->channel = channel;
	send_report(dev, &report);

	recv_report(dev, &report);
	*version = p->version;
	return string(p->name,
		      report.size - API_BASE_SIZE - sizeof(api_info_t));
}

static void process(hid_device *dev, int argc, char **argv)
{
	hid_set_nonblocking(dev, 0);
	uint8_t channel = channels(dev);
	logger->info("{} channels available", channel);

	// Process arguments
	++argv, --argc;
	// Iterate through all channels
	for (uint8_t ch = 0u; ch != channel; ch++) {
		uint16_t version;
		string name = channel_info(dev, ch, &version);
		logger->info("Channel {} (v{:04x}): {}", ch, version, name);

		if (name == "Ping") {
			if (strcmp(*argv, "ping") == 0)
				ping(dev, ch);
		} else if (name == "Keycode") {
			if (strcmp(*argv, "keycode") == 0) {
				if (argv++, !--argc)
					return;
				uint8_t btn = atoi(*argv);
				if (argv++, !--argc)
					return;
				uint8_t code = atoi(*argv);
				keycode_update(dev, ch, btn, code);
			}
		}
	}

#if 0
	if (strcmp(*argv, "flash") == 0) {
		if (argv++, !--argc)
			return;
		flash(dev, *argv);
	} else if (strcmp(*argv, "reset") == 0) {
		reset(dev);
	} else
#endif
}

int main(int argc, char **argv)
{
	logger->set_pattern("[%T %t/%l]: %v");
	logger->set_level(spdlog::level::info);

#ifdef __GNUC__
	auto name = basename(argv[0]);
#else
	auto len = strlen(argv[0]);
	auto fname = new char[len];
	auto ext = new char[len];
	_splitpath(argv[0], NULL, NULL, fname, ext);
	string name(string(fname) + string(ext));
	delete fname;
	delete ext;
#endif

	if (argc <= 1) {
		clog << "Usage: " << name << " operation [arguments]" << endl << endl;
		clog << "Available operations:" << endl;
		clog << "  ping               | Check device signature and version info" << endl;
		clog << "  reset              | Reset target device" << endl;
		clog << "  flash program.hex  | Flash program.hex to target devices" << endl;
		clog << "  keycode btn code   | Update keycode of button `btn' to `code'" << endl;
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
#ifdef WIN32
		if (info->usage_page != HID_USAGE_PAGE || info->usage != HID_USAGE)
			continue;
#endif
		auto mf = info->manufacturer_string ? info->manufacturer_string : L"(null)";
		auto prod = info->product_string ? info->product_string : L"(null)";
		auto sn = info->serial_number ? info->serial_number : L"(null)";
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
