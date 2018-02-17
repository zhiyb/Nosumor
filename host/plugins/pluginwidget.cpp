#include "pluginwidget.h"
#include "plugin.h"

PluginWidget::PluginWidget(uint8_t channel, QWidget *parent) :
	QWidget(parent), channel(channel)
{
}

void PluginWidget::send(hid_device *dev, api_report_t *rp)
{
	Plugin::send(dev, channel, rp);
}

void PluginWidget::recv(hid_device *dev, api_report_t *rp)
{
	Plugin::recv(dev, rp);
}
