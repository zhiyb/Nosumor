#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include "devicewidget.h"
#include "hidapi.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);

protected:
	void timerEvent(QTimerEvent *);

private slots:
	void refreshDeviceList();

private:
	QVBoxLayout *devLayout;
	QLabel *lDevices;
};

#endif // MAINWINDOW_H
