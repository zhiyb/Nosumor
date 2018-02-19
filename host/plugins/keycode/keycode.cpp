#include <QtWidgets>
#include <plugin.h>
#include <dev_defs.h>
#include <api_keycode.h>
#include "inputcapture.h"
#include "keycode.h"
#include "usage.h"

Keycode::Keycode(hid_device *dev, uint8_t channel, QWidget *parent) :
	PluginWidget(channel, parent)
{
	this->dev = dev;

	auto layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	bg = new QButtonGroup(this);

	uint8_t num = keysTotal();
	for (uint8_t i = 0; i != num; i++) {
		auto pb = new QPushButton(this);
		bg->addButton(pb, i);
		layout->addWidget(pb);
		updateKeyInfo(i);
	}

	connect(bg, QOverload<int>::of(&QButtonGroup::buttonClicked),
		this, &Keycode::update);
}

uint8_t Keycode::keysTotal()
{
	api_report_t report;
	report.size = 0u;
	send(dev, &report);

	recv(dev, &report);
	return report.payload[0];
}

std::string Keycode::keyInfo(int id, uint8_t *code)
{
	api_report_t report;
	report.size = 1u;
	report.payload[0] = id;
	send(dev, &report);

	recv(dev, &report);
	*code = report.payload[0];
	return std::string((const char *)&report.payload[1], report.size - 1u);
}

void Keycode::updateKeyInfo(int btn)
{
	uint8_t code;
	std::string name = keyInfo(btn, &code);
	// Update button text
	bg->button(btn)->setText(QObject::tr("Key %1: %2")
				 .arg(QString::fromStdString(name))
				 .arg(Usage::keyboardString(code)));
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
	updateKeyInfo(btn);
}
