#ifndef PLUGINPING_H
#define PLUGINPING_H

#include <string>
#include <unordered_map>
#include <api_defs.h>
#include "plugin.h"

class PluginInfo : public Plugin
{
public:
	PluginInfo() : Plugin() {}
	~PluginInfo() {}

	virtual uint16_t version() const {return SW_VERSION;}
	virtual std::string name() const {return "Info";}
	virtual std::string displayName() const {return "Information channel";}
	virtual void *pluginWidget(hid_device *dev, hid_device_info *info,
				   uint8_t channel, void *parent = nullptr);

	uint8_t channelTotal(hid_device *dev);
	std::string channelInfo(hid_device *dev, uint8_t channel, uint16_t *version);
	std::unordered_map<std::string, std::pair<uint16_t, uint8_t> > channels(hid_device *dev);
};

#endif // PLUGINPING_H
