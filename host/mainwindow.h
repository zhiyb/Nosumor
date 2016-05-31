#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include "hidapi.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);

protected:
	void timerEvent(QTimerEvent *);

private:
	void refreshDeviceList();

	QLabel *lDevices;
	hid_device *device;
	struct hid_device_info *devices;
};

#endif // MAINWINDOW_H
