#include <pluginwidget.h>
#include <dev_defs.h>
#include <plugininfo.h>
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
	if (!plugins)
		return false;

	try {
		enumerateChannels();
		foreach (Plugin *plugin, *plugins) {
			auto const &it = channels.find(QString::fromStdString(plugin->name()));
			if (it == channels.end())
				continue;
			if (plugin->version() < it.value().first) {
				layout->addWidget(new QLabel(tr("Plugin %1 outdated: v%2")
							     .arg(QString::fromStdString(plugin->name()))
							     .arg(it.value().first, 4, 16, QChar('0'))));
				continue;
			}

			//qDebug() << "Creating: " << QString::fromStdString(plugin->name());
			auto w = (PluginWidget *)plugin->pluginWidget(dev, info, it.value().second, this);
			if (!w)
				continue;
			connect(w, &PluginWidget::devRemove, this, &DeviceWidget::devRemove);
			layout->addWidget(w);
		}
	} catch (exception &e) {
		layout->addWidget(new QLabel(tr("Error: %1").arg(e.what())));
	}
	return true;
}

bool DeviceWidget::devRefresh()
{
	wchar_t str[256];
	if (hid_get_serial_number_string(dev, str, 256) == 0)
		return true;
	deleteLater();
	return false;
}

void DeviceWidget::devRemove()
{
	emit devRemoved(this);
}

void DeviceWidget::enumerateChannels()
{
	for (auto &it: PluginInfo().channels(dev))
		channels.insert(QString::fromStdString(it.first),
				QPair<uint16_t, uint8_t>(it.second.first, it.second.second));
}
