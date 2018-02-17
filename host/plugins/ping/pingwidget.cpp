#include <plugin.h>
#include <dev_defs.h>
#include <api_ping.h>
#include "pingwidget.h"

PingWidget::PingWidget(hid_device *dev, hid_device_info *info, uint8_t channel, QWidget *parent) :
	PluginWidget(channel, parent)
{
	this->dev = dev;

	QString mf("(null)"), prod("(null)"), sn("(null)");
	if (info->manufacturer_string)
		mf = QString::fromWCharArray(info->manufacturer_string);
	if (info->product_string)
		prod = QString::fromWCharArray(info->product_string);
	if (info->serial_number)
		sn = QString::fromWCharArray(info->serial_number);

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
	api_report_t report;
	report.size = 0u;
	send(dev, &report);

	recv(dev, &report);
	api_pong_t *pong = (api_pong_t *)report.payload;
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
