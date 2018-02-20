#ifndef CONFIG_H
#define CONFIG_H

#include <QtWidgets>
#include <pluginwidget.h>
#include <hidapi.h>

class Config : public PluginWidget
{
	Q_OBJECT
public:
	explicit Config(hid_device *dev, uint8_t channel, QWidget *parent = nullptr);

signals:

public slots:

private slots:
	void updateCheckBox(bool c);

private:
	uint8_t configsTotal();
	std::string configInfo(int id, uint8_t *size, void *p);
	void updateConfig(int id, uint8_t size, uint32_t value);
	void addConfigWidget(int id);

	hid_device *dev;

	QHBoxLayout *layout;
};

#endif // CONFIG_H
