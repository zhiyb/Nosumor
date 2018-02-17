#ifndef DEVICEWIDGET_H
#define DEVICEWIDGET_H

#include <QtWidgets>
#include <hidapi.h>
#include "plugin.h"

class DeviceWidget : public QGroupBox
{
	Q_OBJECT
public:
	explicit DeviceWidget(hid_device_info *info, QWidget *parent = nullptr);
	~DeviceWidget();

	bool devOpen(hid_device_info *info, const QList<Plugin *> *plugins = nullptr);
	// Check device status
	bool devRefresh();

	QString devPath() {return this->path;}

signals:
	void devRemoved(DeviceWidget *);

public slots:

private slots:
	void devRemove();

private:
	void enumerateChannels();

	QHash<QString, QPair<uint16_t, uint8_t> > channels;
	const QList<Plugin *> *plugins;
	QVBoxLayout *layout;
	QLabel *ping;
	QString path;
	hid_device *dev;
};

#endif // DEVICEWIDGET_H
