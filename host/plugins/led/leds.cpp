#include <QtWidgets>
#include <plugin.h>
#include <dev_defs.h>
#include <api_led.h>
#include "leds.h"

LEDs::LEDs(hid_device *dev, uint8_t channel, QWidget *parent) :
	PluginWidget(channel, parent)
{
	this->dev = dev;

	layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	buttons = new QButtonGroup(this);
	connect(buttons, QOverload<int>::of(&QButtonGroup::buttonClicked),
		this, &LEDs::update);

	getInfo();
}

void LEDs::update(int id)
{
	QColor colour = QColorDialog::getColor(ledList[id].qColor(), this);
	if (!colour.isValid())
		return;

	uint32_t size = 2u * ledList[id].elements;
	ledList[id].set(colour);

	api_report_t report;
	report.size = 1u + size;
	report.payload[0] = id;
	memcpy(&report.payload[1], ledList[id].colour, size);
	send(dev, &report);
}

void LEDs::getInfo()
{
	api_report_t report;
	report.size = 0u;
	send(dev, &report);

	recv(dev, &report);
	api_led_info_t *info = (api_led_info_t *)report.payload;
	for (int i = 0; i != info->num; i++) {
		led_t led;
		led.id = i;
		led.bits = info->led[i].type & 0x0f;
		led.elements = info->led[i].type >> 4u;
		led.position = info->led[i].position;
		led.button = new QPushButton;
		ledList.append(led);

		led.button->setFlat(true);
		led.button->setAutoFillBackground(true);
		buttons->addButton(led.button, i);
		layout->addWidget(led.button);

		getColor(i);
	}
}

void LEDs::getColor(int id)
{
	api_report_t report;
	report.size = 1u;
	report.payload[0] = id;
	send(dev, &report);

	recv(dev, &report);
	api_led_config_t *config = (api_led_config_t *)report.payload;
	ledList[id].set(config->clr);
}

QColor LEDs::led_t::qColor() const
{
	return QColor(colour[0] >> (bits - 8u),
			colour[1] >> (bits - 8u),
			colour[2] >> (bits - 8u));
}

void LEDs::led_t::set(QColor clr)
{
	uint16_t colour[3];
	colour[0] = clr.red() << (bits - 8u);
	colour[1] = clr.green() << (bits - 8u);
	colour[2] = clr.blue() << (bits - 8u);
	set(colour);
}

void LEDs::led_t::set(uint16_t *clr)
{
	memcpy(colour, clr, 2u * elements);
	button->setPalette(QPalette(qColor()));
}
