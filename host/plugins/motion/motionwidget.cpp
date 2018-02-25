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
	layout->addLayout(vLayout);
	vLayout->addStretch();

	auto upd = new QCheckBox(tr("Update"));
	vLayout->addWidget(upd);
	auto reset = new QPushButton(tr("Reset"));
	vLayout->addWidget(reset);

	auto gb = new QGroupBox(tr("Quaternion"));
	vLayout->addWidget(gb);

	auto gbLayout = new QVBoxLayout(gb);
	for (int i = 0; i != 4; i++)
		gbLayout->addWidget(quat[i] = new QLabel);

	gb = new QGroupBox(tr("Compass"));
	vLayout->addWidget(gb);

	gbLayout = new QVBoxLayout(gb);
	for (int i = 0; i != 3; i++)
		gbLayout->addWidget(compass[i] = new QLabel);

	vLayout->addStretch();

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
	gl->updateCompass(p->compass);

	quat[0]->setText(tr("a: %1").arg(p->quat[0]));
	quat[1]->setText(tr("b: %1").arg(p->quat[1]));
	quat[2]->setText(tr("c: %1").arg(p->quat[2]));
	quat[3]->setText(tr("d: %1").arg(p->quat[3]));
	compass[0]->setText(tr("x: %1").arg(p->compass[0]));
	compass[1]->setText(tr("y: %1").arg(p->compass[1]));
	compass[2]->setText(tr("z: %1").arg(p->compass[2]));
}
