#include <string>
#include <sstream>
#include <iomanip>
#include "plugininfo.h"

using namespace std;

PLUGIN_EXPORT Plugin *pluginLoad()
{
	return new PluginInfo();
}

void *PluginInfo::pluginWidget(hid_device *dev, hid_device_info *info, uint8_t channel, void *parent)
{
	(void)dev;
	(void)info;
	(void)channel;
	(void)parent;
	return 0;
}

uint8_t PluginInfo::channelTotal(hid_device *dev)
{
	api_report_t report;
	report.size = 1u;
	report.channel = 0u;

	api_info_t *p = (api_info_t *)report.payload;
	p->channel = 0;
	send(dev, 0, &report);

	recv(dev, &report);
	api_info_t *info = (api_info_t *)report.payload;
	if (info->version > version()) {
		stringstream ss;
		ss << "Software outdated: 0x" << std::hex << std::noshowbase
		   << std::setw(4) << std::setfill('0') << info->version;
		throw runtime_error(ss.str());
	}
	return info->channel;
}

std::string PluginInfo::channelInfo(hid_device *dev, uint8_t channel, uint16_t *version)
{
	api_report_t report;
	report.size = 1u;
	report.channel = 0u;

	api_info_t *p = (api_info_t *)report.payload;
	p->channel = channel;
	send(dev, 0, &report);

	recv(dev, &report);
	*version = p->version;
	return string(p->name, report.size - sizeof(api_info_t));
}

std::unordered_map<std::string, std::pair<uint16_t, uint8_t> >
PluginInfo::channels(hid_device *dev)
{
	uint8_t channel = channelTotal(dev);
	std::unordered_map<std::string, std::pair<uint16_t, uint8_t> > map;
	// Iterate through all channels
	for (uint8_t ch = 0u; ch != channel; ch++) {
		uint16_t version;
		string name = channelInfo(dev, ch, &version);
		map.insert(std::pair<std::string, std::pair<uint16_t, uint8_t> >
			   (name, std::make_pair(version, ch)));
	}
	return map;
}
