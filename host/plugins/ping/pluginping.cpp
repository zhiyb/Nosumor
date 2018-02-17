#include "pingwidget.h"
#include "pluginping.h"

PLUGIN_EXPORT Plugin *pluginLoad()
{
	return new PluginPing();
}

void *PluginPing::pluginWidget(hid_device *dev, hid_device_info *info, uint8_t channel, void *parent)
{
	return new PingWidget(dev, info, channel, (QWidget *)parent);
}
