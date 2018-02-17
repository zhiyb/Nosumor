#include "keycode.h"
#include "pluginkeycode.h"

PLUGIN_EXPORT Plugin *pluginLoad()
{
	return new PluginKeycode();
}

void *PluginKeycode::pluginWidget(hid_device *dev, hid_device_info *, uint8_t channel, void *parent)
{
	return new Keycode(dev, channel, (QWidget *)parent);
}
