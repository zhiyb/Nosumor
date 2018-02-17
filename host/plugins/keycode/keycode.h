#ifndef KEYCODE_H
#define KEYCODE_H

#include <pluginwidget.h>
#include <hidapi.h>

class Keycode : public PluginWidget
{
	Q_OBJECT
public:
	explicit Keycode(hid_device *dev, uint8_t channel, QWidget *parent = nullptr);

signals:

public slots:

private slots:
	void update(int id);

private:
	hid_device *dev;
};

#endif // KEYCODE_H
