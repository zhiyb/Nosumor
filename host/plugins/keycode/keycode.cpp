#include <QtWidgets>
#include <dev_defs.h>
#include <plugin.h>
#include "keycode.h"

Keycode::Keycode(hid_device *dev, QWidget *parent) : PluginWidget(parent)
{
	this->dev = dev;

	auto layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	auto bg = new QButtonGroup(this);

	auto pb = new QPushButton(QObject::tr("Update K&L"), this);
	bg->addButton(pb, 1);
	layout->addWidget(pb);

	pb = new QPushButton(QObject::tr("Update K&R"), this);
	bg->addButton(pb, 0);
	layout->addWidget(pb);

	pb = new QPushButton(QObject::tr("Update K&1"), this);
	bg->addButton(pb, 2);
	layout->addWidget(pb);

	pb = new QPushButton(QObject::tr("Update K&2"), this);
	bg->addButton(pb, 3);
	layout->addWidget(pb);

	pb = new QPushButton(QObject::tr("Update K&3"), this);
	bg->addButton(pb, 4);
	layout->addWidget(pb);

	connect(bg, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &Keycode::update);
}

void Keycode::update(int btn)
{
	const QString keys[5] = {tr("right key"), tr("left key"),
				 tr("key 1"), tr("key 2"), tr("key 3")};
	bool ok;
	int code = QInputDialog::getInt(this, tr("Update keycode..."),
					tr("Please input a new keycode for %1:").arg(keys[btn]),
					0, 0, 255, 1, &ok);
	if (!ok)
		return;

	vendor_report_t report;
	report.id = HID_REPORT_ID;
	report.size = VENDOR_REPORT_BASE_SIZE + 2u;
	report.type = KeycodeUpdate;
	report.payload[0] = btn;
	report.payload[1] = code;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
}
