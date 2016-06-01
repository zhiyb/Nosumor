#include "mainwindow.h"
#include "hidapi.h"
#include "usb_desc.h"

#define REFRESH_RATE	1000

#define USB_USAGE_PAGE	0xff39	// Vendor
#define USB_USAGE	0xff	// Vendor

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	qRegisterMetaType<vendor_in_t>("vendor_in_t");

	QWidget *w = new QWidget(this);
	setCentralWidget(w);

	devLayout = new QVBoxLayout(w);
	devLayout->addWidget(lDevices = new QLabel());

	refreshDeviceList();
	startTimer(REFRESH_RATE);
}

void MainWindow::timerEvent(QTimerEvent *)
{
	refreshDeviceList();
}

void MainWindow::refreshDeviceList()
{
	int num = 0;
	hid_device_info *devs = hid_enumerate(USB_VID, USB_PID);
	for (hid_device_info *dev = devs; dev; dev = dev->next) {
		if (dev->usage_page != USB_USAGE_PAGE || dev->usage != USB_USAGE)
			continue;
		if (!findChild<DeviceWidget *>(dev->path)) {
			DeviceWidget *device = new DeviceWidget(dev->path);
			connect(device, &DeviceWidget::update, this, &MainWindow::refreshDeviceList);
			devLayout->addWidget(device);
		}
		num++;
	}
	hid_free_enumeration(devs);
	lDevices->setText(tr("%1 device(s) found.").arg(num));
}
