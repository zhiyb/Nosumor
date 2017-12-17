#ifndef DEVICEWIDGET_H
#define DEVICEWIDGET_H

#include <QtWidgets>
#include <hidapi.h>

class DeviceWidget : public QGroupBox
{
	Q_OBJECT
public:
	explicit DeviceWidget(hid_device_info *info, QWidget *parent = nullptr);
	~DeviceWidget();

	bool open();
	bool refresh();

signals:

public slots:

private:
	QString path;
	hid_device *dev;
};

#endif // DEVICEWIDGET_H
