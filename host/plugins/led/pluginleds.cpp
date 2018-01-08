#include "leds.h"
#include "pluginleds.h"

PLUGIN_EXPORT Plugin *pluginLoad()
{
	return new PluginLEDs();
}

void *PluginLEDs::pluginWidget(hid_device *dev, hid_device_info *, void *parent)
{
	return new LEDs(dev, (QWidget *)parent);
}
