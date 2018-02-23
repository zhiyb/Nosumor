#ifndef PINGWIDGET_H
#define PINGWIDGET_H

#include <QtWidgets>
#include <pluginwidget.h>
#include <hidapi.h>
#include "motionglwidget.h"

class MotionWidget : public PluginWidget
{
	Q_OBJECT
public:
	explicit MotionWidget(hid_device *dev, hid_device_info *info,
			      uint8_t channel, QWidget *parent = nullptr);

signals:

public slots:

protected:
	void timerEvent(QTimerEvent *event);

private slots:
	void enableTimer(bool e);

private:
	void update();

	hid_device *dev;

	MotionGLWidget *gl;
	int timer;
};

#endif // PINGWIDGET_H
