#ifndef PLUGINKEYCODE_H
#define PLUGINKEYCODE_H

#include <QtWidgets>
#include "plugin.h"

class PluginKeycode : public Plugin
{
public:
	PluginKeycode() : Plugin() {}
	~PluginKeycode() {}

	virtual std::string name() const {return "Keycode settings";}
	virtual void *pluginWidget(hid_device *dev, hid_device_info *info, void *parent = nullptr);
};

#endif // PLUGINKEYCODE_H
