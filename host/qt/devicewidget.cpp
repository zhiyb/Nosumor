#include "devicewidget.h"

DeviceWidget::DeviceWidget(hid_device_info *info, QWidget *parent) : QGroupBox(parent)
{
	dev = 0;
	path = info->path;
	auto mf = QString::fromWCharArray(info->manufacturer_string ?: L"(null)");
	auto prod = QString::fromWCharArray(info->product_string ?: L"(null)");
	auto sn = QString::fromWCharArray(info->serial_number ?: L"(null)");

	auto tPath = path;
	setTitle(tr("Device %1").arg(tPath.replace('&', "&&")));

	auto layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("Manufacturer:\t%1\n"
					"Product ID:\t%2\n"
					"Serial number:\t%3").arg(mf).arg(prod).arg(sn)));
}

DeviceWidget::~DeviceWidget()
{
	if (dev)
		hid_close(dev);
}

bool DeviceWidget::open()
{
	if (!(dev = hid_open_path(path.toLocal8Bit()))) {
		QMessageBox::warning(this, tr("Warning"), tr("Cannot open device %1").arg(path));
		deleteLater();
		return false;
	}
	return true;
}

bool DeviceWidget::refresh()
{
	wchar_t str[256];
	if (hid_get_serial_number_string(dev, str, 256) == 0)
		return true;
	QMessageBox::information(this, tr("Info"), tr("Device %1 removed").arg(path));
	deleteLater();
	return false;
}
