#ifndef PLUGIN_H
#define PLUGIN_H

#include <string>
#include <hidapi.h>
#include <api_defs.h>

#if defined(WIN32)
#define PLUGIN_EXPORT	extern "C" __declspec(dllexport)
#else
#define PLUGIN_EXPORT	extern "C"
#endif

class Plugin
{
public:
	Plugin() {}
	virtual ~Plugin() {}

	virtual std::string name() const = 0;
	virtual std::string displayName() const = 0;
	virtual void *pluginWidget(hid_device *dev, hid_device_info *info,
				   uint8_t channel, void *parent = nullptr) = 0;

	static void send(hid_device *dev, uint8_t channel, api_report_t *rp);
	static void recv(hid_device *dev, api_report_t *rp);
};

typedef Plugin *(*pluginLoad_t)();

#endif // PLUGIN_H
