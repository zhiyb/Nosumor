#ifndef PLUGINFLASH_H
#define PLUGINFLASH_H

#include <QtWidgets>
#include "plugin.h"

class PluginFlash : public Plugin
{
public:
	PluginFlash() : Plugin() {}
	~PluginFlash() {}

	virtual uint16_t version() const {return 0x0001;}
	virtual std::string name() const {return "Flash";}
	virtual std::string displayName() const {return "Firmware Flash";}
	virtual void *pluginWidget(hid_device *dev, hid_device_info *info,
				   uint8_t channel, void *parent = nullptr);
};

#endif // PLUGINFLASH_H
