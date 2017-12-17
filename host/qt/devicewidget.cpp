#include <dev_defs.h>
#include "devicewidget.h"

#define TIMEOUT_MS	5000

using namespace std;

DeviceWidget::DeviceWidget(hid_device_info *info, QWidget *parent) : QGroupBox(parent)
{
	dev = 0;
	path = info->path;
	auto mf = QString::fromWCharArray(info->manufacturer_string ?: L"(null)");
	auto prod = QString::fromWCharArray(info->product_string ?: L"(null)");
	auto sn = QString::fromWCharArray(info->serial_number ?: L"(null)");

	auto tPath = path;
	setTitle(tr("Device %1").arg(tPath.replace('&', "&&")));

	auto layout = new QVBoxLayout(this);
	auto hLayout = new QHBoxLayout();
	layout->addLayout(hLayout);
	hLayout->addWidget(ping = new QLabel());
	hLayout->addWidget(new QLabel(tr("Manufacturer:\t%1\n"
					"Product ID:\t%2\n"
					"Serial number:\t%3").arg(mf).arg(prod).arg(sn)));

	hLayout = new QHBoxLayout();
	layout->addLayout(hLayout);
	auto pb = new QPushButton(tr("Reset"));
	connect(pb, &QPushButton::clicked, this, &DeviceWidget::devReset);
	hLayout->addWidget(pb);
	pb = new QPushButton(tr("Flash"));
	connect(pb, &QPushButton::clicked, this, &DeviceWidget::devFlash);
	hLayout->addWidget(pb);
}

DeviceWidget::~DeviceWidget()
{
	if (dev)
		hid_close(dev);
}

bool DeviceWidget::devOpen()
{
	if (!(dev = hid_open_path(path.toLocal8Bit()))) {
		QMessageBox::warning(this, tr("Warning"), tr("Cannot open device %1").arg(path));
		deleteLater();
		return false;
	}
	devPing();
	return true;
}

bool DeviceWidget::devRefresh()
{
	wchar_t str[256];
	if (hid_get_serial_number_string(dev, str, 256) == 0)
		return true;
	QMessageBox::information(this, tr("Info"), tr("Device %1 removed").arg(path));
	deleteLater();
	return false;
}

void DeviceWidget::devReset()
{
	vendor_report_t report;
	report.id = HID_REPORT_ID;
	report.size = VENDOR_REPORT_BASE_SIZE;
	report.type = FlashReset;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	report.type = FlashStart;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	emit devRemoved(this);
}

void DeviceWidget::devFlash()
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
	readReport(dev, report.raw);
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
	emit devRemoved(this);
}

void DeviceWidget::readReport(hid_device *dev, void *p)
{
	int ret = hid_read_timeout(dev, (uint8_t *)p, VENDOR_REPORT_SIZE, TIMEOUT_MS);
	if (ret == 0)
		throw runtime_error("Report IN timed out");
	else if (ret != VENDOR_REPORT_SIZE) {
		if (ret > 0)
			throw runtime_error("Incorrect report size: " + ret);
		else
			throw runtime_error("Report IN error: " + ret);
	}
}

void DeviceWidget::devPing()
{
	vendor_report_t report;
	report.id = HID_REPORT_ID;
	report.size = VENDOR_REPORT_BASE_SIZE;
	report.type = Ping;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	readReport(dev, report.raw);
	pong_t *pong = (pong_t *)report.payload;
	ping->setText(tr("Hardware version:\t%1\n"
			 "Software version:\t%2\n"
			 "Device UID:\t%3%4%5\n"
			 "Flash size:\t%6 KiB")
		      .arg(pong->hw_ver, 4, 16, QChar('0'))
		      .arg(pong->sw_ver, 4, 16, QChar('0'))
		      .arg(pong->uid[2], 8, 16, QChar('0'))
		      .arg(pong->uid[1], 8, 16, QChar('0'))
		      .arg(pong->uid[0], 8, 16, QChar('0'))
		      .arg(pong->fsize));
}
