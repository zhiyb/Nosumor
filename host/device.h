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

signals:
	void update();
	void dataReceived(vendor_in_t data);

protected:
	void run();

private:
	QString devPath;
	hid_device *dev;
	volatile bool stopReq;
};

#endif // DEVICE_H
