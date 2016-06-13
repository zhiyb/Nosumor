#include "mainwindow.h"
#include "hidapi.h"
#include <QApplication>
#include <QMessageBox>

volatile int fatal = false;

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	if (hid_init() != 0) {
		QMessageBox::critical(0, QObject::tr("Error"),
				      QObject::tr("Cannot initialise hidapi"));
		return -1;
	}

	QSurfaceFormat format;
	format.setRenderableType(QSurfaceFormat::OpenGL);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setVersion(3, 3);
	format.setOptions(0);
	format.setStereo(false);
	format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	format.setSwapInterval(1);
	format.setAlphaBufferSize(0);
	format.setRedBufferSize(8);
	format.setGreenBufferSize(8);
	format.setBlueBufferSize(8);
	format.setDepthBufferSize(0);
	format.setStencilBufferSize(0);
	format.setSamples(1);
	QSurfaceFormat::setDefaultFormat(format);

	MainWindow w;
	w.show();

	if (fatal)
		return fatal;
	int app = a.exec();
	hid_exit();
	return app;
}
