#ifndef PLUGINCONFIG_H
#define PLUGINCONFIG_H

#include <QtWidgets>
#include "plugin.h"

class PluginConfig : public Plugin
{
public:
	PluginConfig() : Plugin() {}
	~PluginConfig() {}

	virtual uint16_t version() const {return 0x0001;}
	virtual std::string name() const {return "Config";}
	virtual std::string displayName() const {return "System Options";}
	virtual void *pluginWidget(hid_device *dev, hid_device_info *info,
				   uint8_t channel, void *parent = nullptr);
};

#endif // PLUGINCONFIG_H
