#include <QtWidgets>
#include <dev_defs.h>
#include <plugin.h>
#include "i2c.h"

I2C::I2C(hid_device *dev, QWidget *parent) : PluginWidget(parent)
{
	this->dev = dev;

	auto *layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	layout->addWidget(new QLabel(tr("Addr:")));
	layout->addWidget(addr = new QLineEdit("0x30"));
	layout->addWidget(read = new QCheckBox(tr("&Read")));
	layout->addWidget(new QLabel(tr("Data:")));
	layout->addWidget(data = new QLineEdit, 1);
	auto pb = new QPushButton(tr("&Send"));
	layout->addWidget(pb);

	connect(addr, &QLineEdit::returnPressed, data,
		QOverload<>::of(&QLineEdit::setFocus));
	connect(read, &QCheckBox::toggled, this, &I2C::readToggled);
	connect(data, &QLineEdit::returnPressed, pb, &QPushButton::click);
	connect(pb, &QPushButton::clicked, this, &I2C::send);
}

void I2C::readToggled(bool e)
{
	uint8_t a = addr->text().toUInt(nullptr, 0);
	a = (a & 0xfe) | (e ? 0x01 : 0x00);
	addr->setText("0x" + QString::number(a, 16));
}

void I2C::send()
{
	uint8_t a = addr->text().toUInt(nullptr, 0);
	QStringList d = data->text().split(QRegularExpression(",|\\s"),
					   QString::SkipEmptyParts);
	QVector<uint8_t> values;
	foreach (const QString &s, d) {
		bool ok;
		auto v = s.toUInt(&ok, 0);
		if (!ok) {
			QMessageBox::warning(this, tr("I2C data error"),
					     tr("Integer conversion failed: %1")
					     .arg(s));
			return;
		}
		if (v >= 256) {
			QMessageBox::warning(this, tr("I2C data error"),
					     tr("Integer overflow: %1")
					     .arg(v));
			return;
		}
		values.append(v);
	}

	vendor_report_t report;
	report.id = HID_REPORT_ID;
	report.size = VENDOR_REPORT_BASE_SIZE + 1u + values.size();
	report.type = I2CData;
	report.payload[0] = a;
	memcpy(&report.payload[1], values.data(), values.size());
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);

	Plugin::readReport(dev, report.raw);
	a = report.payload[0];
	uint8_t v = report.payload[1];
	QMessageBox::information(this, tr("I2C reply"),
				 tr("I2C reply from 0x%1: 0x%2")
				 .arg(a, 2, 16, QChar('0'))
				 .arg(v, 2, 16, QChar('0')));
}
