#include <string>
#include <sstream>
#include <QtWidgets>
#include <dev_defs.h>
#include <plugin.h>
#include "config.h"

using namespace std;

Config::Config(hid_device *dev, uint8_t channel, QWidget *parent) :
	PluginWidget(channel, parent)
{
	this->dev = dev;

	layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	uint8_t num = configsTotal();
	for (uint8_t i = 0; i != num; i++)
		addConfigWidget(i);

	layout->addStretch();
}

void Config::updateCheckBox(bool c)
{
	bool ok = true;
	int id = sender()->property("c_id").toInt(&ok);
	if (!ok)
		throw runtime_error("Invalid configuration ID");
	int size = sender()->property("c_size").toInt(&ok);
	if (!ok)
		throw runtime_error("Invalid configuration size");

	uint32_t value = c;
	updateConfig(id, size, value);
}

uint8_t Config::configsTotal()
{
	api_report_t report;
	report.size = 0u;
	send(dev, &report);

	recv(dev, &report);
	return report.payload[0];
}

std::string Config::configInfo(int id, uint8_t *size, void *p)
{
	api_report_t report;
	report.size = 1u;
	report.payload[0] = id;
	send(dev, &report);

	recv(dev, &report);
	uint8_t s = report.payload[0];
	if (s > 4u) {
		stringstream ss;
		ss << "Invalid configuration size: " << (uint32_t)s;
		throw runtime_error(ss.str());
	}
	*size = s;
	s = s ?: 1u;
	memcpy(p, &report.payload[1], s);
	return string((const char *)&report.payload[1u + s],
			report.size - s - 1u);
}

void Config::updateConfig(int id, uint8_t size, uint32_t value)
{
	api_report_t report;
	size = size ?: 1u;
	report.size = 1u + size;
	report.payload[0] = id;
	memcpy(&report.payload[1], &value, size);
	send(dev, &report);
}

void Config::addConfigWidget(int id)
{
	uint8_t size = 0;
	uint32_t value = 0;
	QString name = QString::fromStdString(configInfo(id, &size, &value));
	QWidget *w = 0;

	switch (size) {
	case 0: {
		auto cb = new QCheckBox(name);
		cb->setChecked(value);
		connect(cb, &QCheckBox::toggled, this, &Config::updateCheckBox);
		w = cb;
		break;
	}
	default: {
		stringstream ss;
		ss << "Unsupported configuration size: " << (uint32_t)size;
		throw runtime_error(ss.str());
	}
	}

	w->setProperty("c_id", id);
	w->setProperty("c_size", size);
	layout->addWidget(w);
}
