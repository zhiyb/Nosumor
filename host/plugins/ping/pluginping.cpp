#include "pingwidget.h"
#include "pluginping.h"

PLUGIN_EXPORT Plugin *pluginLoad()
{
	return new PluginPing();
}

void *PluginPing::pluginWidget(hid_device *dev, hid_device_info *info, void *parent)
{
	return new PingWidget(dev, info, (QWidget *)parent);
}
