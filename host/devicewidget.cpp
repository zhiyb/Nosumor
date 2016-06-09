#include "devicewidget.h"

DeviceWidget::DeviceWidget(const char *path, QWidget *parent)
	: QGroupBox(parent), device(path, this)
{
	if (!device.valid()) {
		// Prevent possible dead loop
		//emit update();
		return;
	}

	setObjectName(path);
	setTitle(QString(path).replace('&', "&&"));

	QVBoxLayout *vLayout = new QVBoxLayout(this);
	vLayout->addWidget(lwEvents = new QListWidget);

	connect(&device, &Device::dataReceived, this, &DeviceWidget::dataReceived);
	connect(&device, &Device::update, this, &DeviceWidget::update);
	connect(&device, &QThread::finished, this, &QObject::deleteLater);
	device.start();
}

DeviceWidget::~DeviceWidget()
{
	if (device.isRunning()) {
		device.stop();
		device.quit();
		device.wait();
	}
}

void DeviceWidget::dataReceived(vendor_in_t data)
{
	lwEvents->addItem(QString("%1, %2").arg(data.timestamp).arg(data.status, 0, 2));
	lwEvents->scrollToBottom();
}
