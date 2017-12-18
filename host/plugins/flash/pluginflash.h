#ifndef PLUGINFLASH_H
#define PLUGINFLASH_H

#include <QtWidgets>
#include "plugin.h"

class PluginFlash : public Plugin
{
public:
	PluginFlash() : Plugin() {}
	~PluginFlash() {}

	virtual std::string name() const {return "Firmware Flash";}
	virtual void *pluginWidget(hid_device *dev, hid_device_info *info, void *parent = nullptr);
};

#endif // PLUGINFLASH_H
