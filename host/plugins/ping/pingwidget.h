#ifndef PINGWIDGET_H
#define PINGWIDGET_H

#include <QtWidgets>
#include <pluginwidget.h>
#include <hidapi.h>

class PingWidget : public PluginWidget
{
	Q_OBJECT
public:
	explicit PingWidget(hid_device *dev, hid_device_info *info, QWidget *parent = nullptr);

signals:

public slots:

private:
	void devPing();

	QLabel *ping;
	hid_device *dev;
};

#endif // PINGWIDGET_H
