#include "pingwidget.h"
#include "pluginping.h"

PLUGIN_EXPORT Plugin *pluginLoad()
{
	return new PluginFlash();
}

void *PluginFlash::pluginWidget(hid_device *dev, hid_device_info *info, void *parent)
{
	return new PingWidget(dev, info, (QWidget *)parent);
}
