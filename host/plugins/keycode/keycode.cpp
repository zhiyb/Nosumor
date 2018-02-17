#include <QtWidgets>
#include <plugin.h>
#include <dev_defs.h>
#include <api_keycode.h>
#include "inputcapture.h"
#include "keycode.h"

Keycode::Keycode(hid_device *dev, uint8_t channel, QWidget *parent) :
	PluginWidget(channel, parent)
{
	this->dev = dev;

	auto layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	auto bg = new QButtonGroup(this);

	auto pb = new QPushButton(QObject::tr("Update K&L"), this);
	bg->addButton(pb, 0);
	layout->addWidget(pb);

	pb = new QPushButton(QObject::tr("Update K&R"), this);
	bg->addButton(pb, 1);
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
	uint16_t code;
	auto m = QApplication::queryKeyboardModifiers();
	if (m & Qt::ControlModifier) {
		bool ok;
		code = QInputDialog::getInt(this, tr("Update keycode"),
					    tr("Input a keycode:"),
					    0, 0, 255, 1, &ok);
		if (!ok)
			return;
	} else {
		InputCapture ic(this);
		if (ic.exec() == QDialog::Rejected)
			return;
		code = ic.usage();
		if (code == 0)
			return;
	}

	api_report_t report;
	report.size = sizeof(api_keycode_t);

	api_keycode_t *p = (api_keycode_t *)report.payload;
	p->btn = btn;
	p->code = code;

	send(dev, &report);
}
