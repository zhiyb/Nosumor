#ifndef I2CS_H
#define I2CS_H

#include <QtWidgets>
#include <pluginwidget.h>
#include <hidapi.h>

class I2C : public PluginWidget
{
	Q_OBJECT
public:
	explicit I2C(hid_device *dev, QWidget *parent = nullptr);

signals:

public slots:

private slots:
	void readToggled(bool e);
	void send();

private:
	hid_device *dev;

	QLineEdit *addr, *data;
	QCheckBox *read;
};

#endif // I2C_H
