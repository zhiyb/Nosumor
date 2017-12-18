#include <pluginwidget.h>
#include <dev_defs.h>
#include "devicewidget.h"

#define TIMEOUT_MS	5000

using namespace std;

DeviceWidget::DeviceWidget(hid_device_info *info, QWidget *parent) : QGroupBox(parent)
{
	dev = 0;
	auto tPath = path = info->path;
	setTitle(tr("Device %1").arg(tPath.replace('&', "&&")));
	layout = new QVBoxLayout(this);
}

DeviceWidget::~DeviceWidget()
{
	if (dev)
		hid_close(dev);
}

bool DeviceWidget::devOpen(hid_device_info *info, const QList<Plugin *> *plugins)
{
	if (!(dev = hid_open_path(path.toLocal8Bit()))) {
		QMessageBox::warning(this, tr("Warning"), tr("Cannot open device %1").arg(path));
		deleteLater();
		return false;
	}
	this->plugins = plugins;
	if (plugins)
		foreach (Plugin *plugin, *plugins) {
			//qDebug() << "Creating: " << QString::fromStdString(plugin->name());
			auto w = (PluginWidget *)plugin->pluginWidget(dev, info, this);
			if (!w)
				continue;
			connect(w, &PluginWidget::devRemove, this, &DeviceWidget::devRemove);
			layout->addWidget(w);
		}
	return true;
}

bool DeviceWidget::devRefresh()
{
	wchar_t str[256];
	if (hid_get_serial_number_string(dev, str, 256) == 0)
		return true;
	QMessageBox::information(this, tr("Info"), tr("Device %1 removed").arg(path));
	deleteLater();
	return false;
}

void DeviceWidget::devRemove()
{
	emit devRemoved(this);
}

void DeviceWidget::readReport(hid_device *dev, void *p)
{
	int ret = hid_read_timeout(dev, (uint8_t *)p, VENDOR_REPORT_SIZE, TIMEOUT_MS);
	if (ret == 0)
		throw runtime_error("Report IN timed out");
	else if (ret != VENDOR_REPORT_SIZE) {
		if (ret > 0)
			throw runtime_error("Incorrect report size: " + ret);
		else
			throw runtime_error("Report IN error: " + ret);
	}
}
