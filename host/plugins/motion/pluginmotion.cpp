#include "motionwidget.h"
#include "pluginmotion.h"

PLUGIN_EXPORT Plugin *pluginLoad()
{
	return new PluginMotion();
}

void *PluginMotion::pluginWidget(hid_device *dev, hid_device_info *info, uint8_t channel, void *parent)
{
	return new MotionWidget(dev, info, channel, (QWidget *)parent);
}
