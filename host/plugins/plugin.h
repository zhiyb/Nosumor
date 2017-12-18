#ifndef PLUGIN_H
#define PLUGIN_H

#include <string>
#include <hidapi.h>

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
	virtual void *pluginWidget(hid_device *dev, hid_device_info *info, void *parent = nullptr) = 0;

	static void readReport(hid_device *dev, void *p);
};

typedef Plugin *(*pluginLoad_t)();

#endif // PLUGIN_H
