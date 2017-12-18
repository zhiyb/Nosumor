#include "keycode.h"
#include "pluginkeycode.h"

PLUGIN_EXPORT Plugin *pluginLoad()
{
	return new PluginKeycode();
}

void *PluginKeycode::pluginWidget(hid_device *dev, hid_device_info *info, void *parent)
{
	return new Keycode(dev, (QWidget *)parent);
}
