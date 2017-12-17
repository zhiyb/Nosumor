#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include <hidapi.h>
#include "devicewidget.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

	int init();

signals:

public slots:

private slots:
	void devRefresh();
	void devRemoved(DeviceWidget *dev);

private:
	bool devOpen(hid_device_info *info);

	QHash<QString, DeviceWidget *> devMap;
	QVBoxLayout *layout;
	QLabel *devCount;
};

#endif // MAINWINDOW_H
