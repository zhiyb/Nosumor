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

	bool devOpen();
	bool devRefresh();

	QString devPath() {return this->path;}

signals:
	void devRemoved(DeviceWidget *);

public slots:

private slots:
	void devReset();
	void devFlash();

private:
	void readReport(hid_device *dev, void *p);
	void devPing();

	QLabel *ping;
	QString path;
	hid_device *dev;
};

#endif // DEVICEWIDGET_H
