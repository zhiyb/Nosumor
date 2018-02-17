#ifndef FLASH_H
#define FLASH_H

#include <pluginwidget.h>
#include <hidapi.h>

class Flash : public PluginWidget
{
	Q_OBJECT
public:
	explicit Flash(hid_device *dev, uint8_t channel, QWidget *parent = nullptr);

signals:

public slots:
	void reset();
	void flash();

private:
	hid_device *dev;
};

#endif // FLASH_H
