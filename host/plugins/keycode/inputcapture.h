#ifndef INPUTCAPTURE_H
#define INPUTCAPTURE_H

#include <QDialog>
#include <QtWidgets>

class InputCapture : public QDialog
{
	Q_OBJECT
public:
	InputCapture(QWidget *parent);
	uint8_t usage() const {return _usage;}

protected:
	void keyPressEvent(QKeyEvent *e);
	void keyReleaseEvent(QKeyEvent *e);

private:
	QLabel *label;
	int keycnt;
	uint8_t _usage;
};

#endif // INPUTCAPTURE_H
