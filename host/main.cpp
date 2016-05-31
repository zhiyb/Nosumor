#include "mainwindow.h"
#include "hidapi.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	if (hid_init() != 0) {
		QMessageBox::critical(0, QObject::tr("Error"),
				      QObject::tr("Cannot initialise hidapi"));
		return -1;
	}

	MainWindow w;
	w.show();

	int app = a.exec();
	hid_exit();
	return app;
}
