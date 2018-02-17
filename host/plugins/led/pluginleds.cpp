#include "leds.h"
#include "pluginleds.h"

PLUGIN_EXPORT Plugin *pluginLoad()
{
	return new PluginLEDs();
}

void *PluginLEDs::pluginWidget(hid_device *dev, hid_device_info *, uint8_t channel, void *parent)
{
	return new LEDs(dev, channel, (QWidget *)parent);
}
