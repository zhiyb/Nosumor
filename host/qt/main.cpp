#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MainWindow w;
	int ret;
	if ((ret = w.init()) != 0)
		return ret;
	w.show();
	return a.exec();
}
