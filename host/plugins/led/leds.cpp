#include <QtWidgets>
#include <dev_defs.h>
#include <plugin.h>
#include "leds.h"

LEDs::LEDs(hid_device *dev, QWidget *parent) : PluginWidget(parent)
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

	vendor_report_t report;
	report.id = HID_REPORT_ID;
	report.size = VENDOR_REPORT_BASE_SIZE + size + 1u;
	report.type = LEDConfig;
	report.payload[0] = id | 0x80;
	memcpy(&report.payload[1], ledList[id].colour, size);
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
}

void LEDs::getInfo()
{
	vendor_report_t report;
	report.id = HID_REPORT_ID;
	report.size = VENDOR_REPORT_BASE_SIZE;
	report.type = LEDInfo;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);

	Plugin::readReport(dev, report.raw);
	led_info_t *info = (led_info_t *)report.payload;
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
	}

	for (int i = 0; i != info->num; i++) {
		auto &led = ledList[i];
		getColor(i, led.colour);
	}
}

void LEDs::getColor(int id, uint16_t *clr)
{
	vendor_report_t report;
	report.id = HID_REPORT_ID;
	report.size = VENDOR_REPORT_BASE_SIZE + 1u;
	report.type = LEDConfig;
	report.payload[0] = id;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);

	Plugin::readReport(dev, report.raw);
	led_config_t *config = (led_config_t *)report.payload;
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
	qDebug() << colour[0] << colour[1] << colour[2];
	button->setPalette(QPalette(qColor()));
}
