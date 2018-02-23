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

	layout->addWidget(gl = new MotionGLWidget, 1);

	auto vLayout = new QVBoxLayout;
	auto upd = new QCheckBox(tr("Update"));
	vLayout->addWidget(upd);
	auto reset = new QPushButton(tr("Reset"));
	vLayout->addWidget(reset);
	layout->addLayout(vLayout);

	connect(upd, &QCheckBox::toggled, this, &MotionWidget::enableTimer);
	connect(reset, &QPushButton::clicked, gl, &MotionGLWidget::reset);

	update();
	upd->setChecked(true);
}

void MotionWidget::timerEvent(QTimerEvent *event)
{
	try {
		update();
	} catch (std::exception &e) {
		emit devRemove();
	}
}

void MotionWidget::enableTimer(bool e)
{
	if (e) {
		timer = startTimer(50);
	} else {
		killTimer(timer);
		timer = 0;
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
