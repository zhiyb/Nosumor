#ifndef PLUGINI2C_H
#define PLUGINI2C_H

#include <QtWidgets>
#include "plugin.h"

class PluginI2C : public Plugin
{
public:
	PluginI2C() : Plugin() {}
	~PluginI2C() {}

	virtual uint16_t version() const {return 0x0001;}
	virtual std::string name() const {return "I2C";}
	virtual std::string displayName() const {return "I2C debugging";}
	virtual void *pluginWidget(hid_device *dev, hid_device_info *info,
				   uint8_t channel, void *parent = nullptr);
};

#endif // PLUGINI2C_H
