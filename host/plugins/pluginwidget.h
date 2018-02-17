#ifndef PLUGINWIDGET_H
#define PLUGINWIDGET_H

#include <QWidget>
#include <hidapi.h>
#include <api_defs.h>

class PluginWidget : public QWidget
{
	Q_OBJECT
public:
	explicit PluginWidget(uint8_t channel, QWidget *parent = nullptr);

signals:
	void devRemove();

public slots:

protected:
	void send(hid_device *dev, api_report_t *rp);
	void recv(hid_device *dev, api_report_t *rp);

private:
	uint8_t channel;
};

#endif // PLUGINWIDGET_H
