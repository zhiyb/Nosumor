#include <dev_defs.h>
#include "mainwindow.h"
#include "devicewidget.h"
#include "plugin.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	QWidget *w = new QWidget(this);
	setCentralWidget(w);
	setWindowTitle(tr("Nosumor"));
	layout = new QVBoxLayout(w);

	QGroupBox *gb = new QGroupBox(tr("Devices"), w);
	layout->addWidget(gb);
	QHBoxLayout *hbl = new QHBoxLayout(gb);
	QPushButton *pb = new QPushButton(tr("&Refresh"));
	connect(pb, &QPushButton::clicked, this, &MainWindow::devRefresh);
	hbl->addWidget(pb);
	hbl->addSpacing(16);
	hbl->addWidget(devCount = new QLabel);
	hbl->addStretch();

	layout->addStretch();
}

MainWindow::~MainWindow()
{
	foreach (auto plugin, plugins)
		delete plugin;
	hid_exit();
}

int MainWindow::init()
{
	int ret;
	if ((ret = hid_init()) != 0) {
		QMessageBox::critical(this, tr("Error"), tr("Error initialising hidapi: %1").arg(ret));
		return 1;
	}

	// Load plugins
	QDir dir = QDir::current();
	if (dir.cd("plugins")) {
		QStringList filters;
		filters << "*.dll" << "*.so";
		foreach (auto info, dir.entryInfoList(filters, QDir::Files))
			loadPlugin(info.absoluteFilePath());
	}

	devRefresh();
	return 0;
}

void MainWindow::devRefresh()
{
	// Refresh existing devices, check for disconnection
	for (auto it = devMap.begin(); it != devMap.end(); it++)
		if (!it.value()->devRefresh())
			devMap.erase(it);
	// Enumerate devices
	int num = 0;
	hid_device_info *devs = hid_enumerate(USB_VID, USB_PID);
	for (hid_device_info *info = devs; info; info = info->next) {
#if defined(WIN32)
		if (info->usage_page != HID_USAGE_PAGE || info->usage != HID_USAGE)
			continue;
#endif
		if (devOpen(info))
			num++;
	}
	hid_free_enumeration(devs);
	devCount->setText(tr("Connected: %1").arg(num));
}

void MainWindow::devRemoved(DeviceWidget *dev)
{
	devMap.remove(dev->devPath());
	delete dev;
}

bool MainWindow::loadPlugin(const QString path)
{
	auto load = (pluginLoad_t)QLibrary::resolve(path, "pluginLoad");
	if (!load) {
		QMessageBox::warning(this, tr("Warning"), tr("Unable to load plugin:\n%1").arg(path));
		return false;
	}
	plugins << load();
	return true;
}

bool MainWindow::devOpen(hid_device_info *info)
{
	if (devMap.contains(info->path))
		return true;
	auto dev = new DeviceWidget(info, this);
	layout->addWidget(dev);
	if (!dev->devOpen(info, &plugins))
		return false;
	connect(dev, &DeviceWidget::devRemoved, this, &MainWindow::devRemoved);
	devMap[info->path] = dev;
	return true;
}
