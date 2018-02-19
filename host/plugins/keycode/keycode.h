#ifndef KEYCODE_H
#define KEYCODE_H

#include <pluginwidget.h>
#include <hidapi.h>
#include <QButtonGroup>

class Keycode : public PluginWidget
{
	Q_OBJECT
public:
	explicit Keycode(hid_device *dev, uint8_t channel, QWidget *parent = nullptr);

signals:

public slots:

private slots:
	uint8_t keysTotal();
	std::string keyInfo(int id, uint8_t *code);
	void updateKeyInfo(int btn);
	void update(int btn);

private:
	hid_device *dev;
	QButtonGroup *bg;
};

#endif // KEYCODE_H
