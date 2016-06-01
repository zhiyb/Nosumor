#ifndef DEVICE_H
#define DEVICE_H

#include <QThread>
#include "hidapi.h"
#include "usb_desc.h"

class Device : public QThread
{
	Q_OBJECT
public:
	explicit Device(const char *path, QObject *parent = 0);
	bool valid() {return dev != 0;}
	void stop() {stopReq = true;}

protected:
	void run();

signals:
	void dataReceived(vendor_in_t data);

private:
	QString devpath;
	hid_device *dev;
	vendor_in_t data;
	volatile bool stopReq;
};

#endif // DEVICE_H
