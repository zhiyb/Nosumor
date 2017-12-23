#include "usage.h"
#include "inputcapture.h"

InputCapture::InputCapture(QWidget *parent) : QDialog(parent)
{
	keycnt = 0;
	setWindowTitle(tr("Input capture"));

	auto *layout = new QVBoxLayout(this);
	label = new QLabel(tr("Press a key..."), this);
	label->setAlignment(Qt::AlignCenter);
	layout->addWidget(label);
}

void InputCapture::keyPressEvent(QKeyEvent *e)
{
	e->accept();
	if (e->isAutoRepeat())
		return;
	keycnt++;
	_usage = Usage::keyboardUsage(e->modifiers(), (Qt::Key)e->key());
	if (_usage != 0)
		label->setText(Usage::keyboardString(_usage));
	else
		label->setText(tr("Keycode unknown (0x%1)")
			       .arg(e->key(), 8, 16, QChar('0')));
}

void InputCapture::keyReleaseEvent(QKeyEvent *e)
{
	e->accept();
	if (e->isAutoRepeat())
		return;
	if (--keycnt == 0 && _usage != 0)
		accept();
	else if (keycnt < 0)
		keycnt = 0;
}
