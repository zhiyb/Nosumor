#include "flash.h"
#include "pluginflash.h"

PLUGIN_EXPORT Plugin *pluginLoad()
{
	return new PluginFlash();
}

void *PluginFlash::pluginWidget(hid_device *dev, hid_device_info *, uint8_t channel, void *parent)
{
	return new Flash(dev, channel, (QWidget *)parent);
}
