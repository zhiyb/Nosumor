#include <QDebug>
#include "device.h"

Device::Device(const char *path, QObject *parent) : QThread(parent)
{
	stopReq = false;
	devpath = path;
	if (!(dev = hid_open_path(path)))
		qDebug() << devpath << "hid_open_path failed:" << path;
}

void Device::run()
{
	if (!dev)
		return;

loop:
	int err = hid_read_timeout(dev, (unsigned char *)&data, HID_REPORT_VENDOR_IN_SIZE, 10);
	switch (err) {
	case HID_REPORT_VENDOR_IN_SIZE:
		emit dataReceived(data);
		goto loop;
	case 0:
		if (stopReq)
			break;
		else
			goto loop;
	case -1:
		qDebug() << devpath << QString::fromWCharArray(hid_error(dev));
		break;
	}
	hid_close(dev);
}
