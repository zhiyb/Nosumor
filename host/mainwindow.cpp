#include "mainwindow.h"
#include "hidapi.h"

#define USB_VID		0x0483
#define USB_PID		0x5750
#define USB_USAGE	0x06	// Keyboard

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	devices = 0;

	QWidget *w = new QWidget;
	setCentralWidget(w);

	QVBoxLayout *vLayout = new QVBoxLayout(w);
	vLayout->addWidget(lDevices = new QLabel());

	refreshDeviceList();
	startTimer(1000);
}

void MainWindow::timerEvent(QTimerEvent *)
{
	refreshDeviceList();
}

void MainWindow::refreshDeviceList()
{
	//qDebug() << __PRETTY_FUNCTION__;
	if (devices)
		hid_free_enumeration(devices);
	devices = hid_enumerate(USB_VID, USB_PID);

	QString text;
	hid_device_info *dev = devices;
	int num = 0;
	while (dev) {
		if (dev->usage == USB_USAGE) {
			text.append(dev->path);
			text.append(tr(", usage page: %1, usage: %2\n")
				    .arg(dev->usage_page).arg(dev->usage));
			num++;
		}
		dev = dev->next;
	}

	lDevices->setText(tr("Devices found: %1\n%2").arg(num).arg(text));
}
