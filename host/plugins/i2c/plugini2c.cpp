#include "i2c.h"
#include "plugini2c.h"

PLUGIN_EXPORT Plugin *pluginLoad()
{
	return new PluginI2C();
}

void *PluginI2C::pluginWidget(hid_device *dev, hid_device_info *, void *parent)
{
	return new I2C(dev, (QWidget *)parent);
}
