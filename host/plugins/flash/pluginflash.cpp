#include "flash.h"
#include "pluginflash.h"

PLUGIN_EXPORT Plugin *pluginLoad()
{
	return new PluginFlash();
}

void *PluginFlash::pluginWidget(hid_device *dev, hid_device_info *, void *parent)
{
	return new Flash(dev, (QWidget *)parent);
}
