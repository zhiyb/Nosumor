#include "config.h"
#include "pluginconfig.h"

PLUGIN_EXPORT Plugin *pluginLoad()
{
	return new PluginConfig();
}

void *PluginConfig::pluginWidget(hid_device *dev, hid_device_info *, uint8_t channel, void *parent)
{
	return new Config(dev, channel, (QWidget *)parent);
}
