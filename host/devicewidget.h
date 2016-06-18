#ifndef DEVICEWIDGET_H
#define DEVICEWIDGET_H

#include <QtWidgets>
#include "device.h"
#include "viewwidget.h"
#include "report.h"

class DeviceWidget : public QGroupBox
{
	Q_OBJECT
public:
	explicit DeviceWidget(const char *path, QWidget *parent = 0);
	~DeviceWidget();
	QString path() const {return objectName();}

signals:
	void update();

private slots:
	void dataReceived(vendor_in_t data);

private:
	QVector<report_t> reports;

	Device device;
	ViewWidget *view;
};

#endif // DEVICEWIDGET_H
