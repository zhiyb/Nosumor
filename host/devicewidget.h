#ifndef DEVICEWIDGET_H
#define DEVICEWIDGET_H

#include <QtWidgets>
#include "device.h"

class DeviceWidget : public QGroupBox
{
	Q_OBJECT
public:
	explicit DeviceWidget(const char *path, QWidget *parent = 0);
	~DeviceWidget();
	QString path() {return objectName();}

private slots:
	void dataReceived(vendor_in_t data);

private:
	QListWidget *lwEvents;
	Device *device;
};

#endif // DEVICEWIDGET_H
