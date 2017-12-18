#include <dev_defs.h>
#include <plugin.h>
#include "pingwidget.h"

PingWidget::PingWidget(hid_device *dev, hid_device_info *info, QWidget *parent) : PluginWidget(parent)
{
	this->dev = dev;

	auto mf = QString::fromWCharArray(info->manufacturer_string ?: L"(null)");
	auto prod = QString::fromWCharArray(info->product_string ?: L"(null)");
	auto sn = QString::fromWCharArray(info->serial_number ?: L"(null)");

	auto layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	auto hLayout = new QHBoxLayout();
	layout->addLayout(hLayout);
	hLayout->addWidget(ping = new QLabel());
	hLayout->addWidget(new QLabel(tr("Manufacturer:\t%1\n"
					"Product ID:\t%2\n"
					"Serial number:\t%3").arg(mf).arg(prod).arg(sn)));
	devPing();
}

void PingWidget::devPing()
{
	vendor_report_t report;
	report.id = HID_REPORT_ID;
	report.size = VENDOR_REPORT_BASE_SIZE;
	report.type = Ping;
	hid_write(dev, report.raw, VENDOR_REPORT_SIZE);
	Plugin::readReport(dev, report.raw);
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
