#include <QtWidgets>
#include <dev_defs.h>
#include <plugin.h>
#include "flash.h"

Flash::Flash(hid_device *dev, QWidget *parent) : PluginWidget(parent)
{
	this->dev = dev;

	auto layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	auto pb = new QPushButton(QObject::tr("&Reset"), this);
	QObject::connect(pb, &QPushButton::clicked, this, &Flash::reset);
	layout->addWidget(pb);

	pb = new QPushButton(QObject::tr("&Flash"), this);
	QObject::connect(pb, &QPushButton::clicked, this, &Flash::flash);
	layout->addWidget(pb);
}

void Flash::reset()
{
	vendor_report_t report;
	report.id = HID_REPORT_ID;
	report.size = VENDOR_REPORT_BASE_SIZE;
	report.type = FlashReset;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	report.type = FlashStart;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	emit devRemove();
}

void Flash::flash()
{
	auto path = QFileDialog::getOpenFileName(this, tr("Select firmware..."), QString(), "Intel HEX files (*.hex);;All files (*.*)");
	if (path.isEmpty())
		return;
	// Open file for read
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly)) {
		QMessageBox::critical(this, tr("Error"), tr("Cannot open file %1").arg(path));
		return;
	}

	vendor_report_t report;
	report.id = HID_REPORT_ID;
	// Reset flashing buffer
	report.size = VENDOR_REPORT_BASE_SIZE;
	report.type = FlashReset;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	// Write file content
	QTextStream ts(&f);
	QString str;
	while (!(ts >> str).atEnd()) {
		QTextStream ts(&str);
		if (ts.read(1) != ":") {
			QMessageBox::critical(this, tr("Error"), tr("Invalid Intel HEX format"));
			return;
		}
		report.size = VENDOR_REPORT_BASE_SIZE;
		report.type = FlashData;
		uint8_t *p = report.payload;
		while (!ts.atEnd()) {
			bool ok;
			unsigned int v = ts.read(2).toUInt(&ok, 16);
			if (!ok) {
				QMessageBox::critical(this, tr("Error"), tr("Invalid Intel HEX content"));
				return;
			}
			*p++ = v;
			report.size++;
		}
		hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	}
	// Check validity
	report.size = VENDOR_REPORT_BASE_SIZE;
	report.type = FlashCheck;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	Plugin::readReport(dev, report.raw);
	if (!report.payload[0]) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid firmware received"));
		return;
	}
	// Start flashing
	report.size = VENDOR_REPORT_BASE_SIZE;
	report.type = FlashStart;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	QMessageBox::information(this, tr("Info"), tr("Firmware flashing started, "
						      "please wait for reconnection..."));
	emit devRemove();
}
