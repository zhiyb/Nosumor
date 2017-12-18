#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include <hidapi.h>
#include "devicewidget.h"
#include "plugin.h"

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
	bool loadPlugin(const QString path);
	bool devOpen(hid_device_info *info);

	QList<Plugin *> plugins;
	QHash<QString, DeviceWidget *> devMap;
	QVBoxLayout *layout;
	QListWidget *pluginList;
	QLabel *devCount;
};

#endif // MAINWINDOW_H
