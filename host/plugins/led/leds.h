#ifndef RGBS_H
#define RGBS_H

#include <QtWidgets>
#include <pluginwidget.h>
#include <hidapi.h>

class LEDs : public PluginWidget
{
	Q_OBJECT
public:
	explicit LEDs(hid_device *dev, uint8_t channel, QWidget *parent = nullptr);

signals:

public slots:

private slots:
	void update(int btn);

private:
	void getInfo();
	void getColor(int id);

	hid_device *dev;

	struct led_t {
		int id;
		int bits;
		int elements;
		uint8_t position;
		uint16_t colour[3];
		QPushButton *button;

		QColor qColor() const;
		void set(QColor clr);
		void set(uint16_t *clr);
	};

	QHBoxLayout *layout;
	QButtonGroup *buttons;
	QList<led_t> ledList;
};

#endif // RGBS_H
