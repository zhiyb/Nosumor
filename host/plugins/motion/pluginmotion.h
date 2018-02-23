#ifndef PLUGINMOTION_H
#define PLUGINMOTION_H

#include <QtWidgets>
#include "plugin.h"

class PluginMotion : public Plugin
{
public:
	PluginMotion() : Plugin() {}
	~PluginMotion() {}

	virtual uint16_t version() const {return 0x0001;}
	virtual std::string name() const {return "Motion";}
	virtual std::string displayName() const {return "Motion Sensor";}
	virtual void *pluginWidget(hid_device *dev, hid_device_info *info,
				   uint8_t channel, void *parent = nullptr);
};

#endif // PLUGINMOTION_H
