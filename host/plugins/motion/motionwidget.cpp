#include <plugin.h>
#include <dev_defs.h>
#include <api_motion.h>
#include "motionwidget.h"

MotionWidget::MotionWidget(hid_device *dev, hid_device_info *info,
			   uint8_t channel, QWidget *parent) :
	PluginWidget(channel, parent)
{
	this->dev = dev;

	auto layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	layout->addWidget(gl = new MotionGLWidget(this));

	update();
	startTimer(50);
}

void MotionWidget::timerEvent(QTimerEvent *event)
{
	try {
		update();
	} catch (std::exception &e) {
		emit devRemove();
	}
}

void MotionWidget::update()
{
	api_report_t report;
	report.size = 0u;
	send(dev, &report);

	recv(dev, &report);
	api_motion_t *p = (api_motion_t *)report.payload;
	gl->updateQuaternion(p->quat);
}
