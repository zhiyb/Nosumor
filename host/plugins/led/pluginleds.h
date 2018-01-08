#ifndef PLUGINLEDS_H
#define PLUGINLEDS_H

#include <QtWidgets>
#include "plugin.h"

class PluginLEDs : public Plugin
{
public:
	PluginLEDs() : Plugin() {}
	~PluginLEDs() {}

	virtual std::string name() const {return "LED configuration";}
	virtual void *pluginWidget(hid_device *dev, hid_device_info *info, void *parent = nullptr);
};

#endif // PLUGINLEDS_H
