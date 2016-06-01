#include <QDebug>
#include "device.h"

// http://www.signal11.us/oss/hidapi/hidapi/doxygen/html/group__API.html

Device::Device(const char *path, QObject *parent)
	: QThread(parent), devPath(path), stopReq(false)
{
	if (!(dev = hid_open_path(path)))
		qDebug() << devPath << "hid_open_path failed";
}

void Device::run()
{
	if (!valid())
		return;

	vendor_in_t data;
loop:
	int err = hid_read_timeout(dev, (unsigned char *)&data, HID_REPORT_VENDOR_IN_SIZE, 100);
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
		qDebug() << devPath << QString::fromWCharArray(hid_error(dev));
		emit update();
		break;
	}
	hid_close(dev);
}
