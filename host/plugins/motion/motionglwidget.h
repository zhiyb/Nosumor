#ifndef MOTIONGLWIDGET_H
#define MOTIONGLWIDGET_H

#include <QtWidgets>
#include <QOpenGLFunctions_3_0>

class MotionGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_0
{
	Q_OBJECT
public:
	explicit MotionGLWidget(QWidget *parent = 0);
	void updateQuaternion(int32_t *q);

public slots:
	void reset();

protected:
	virtual void initializeGL();
	virtual void resizeGL(int w, int h);
	virtual void paintGL();

private:
	GLuint loadShader(GLenum type, const QByteArray& context);

	struct {
		GLuint program;
		struct {
			GLuint vertex, colour;
			GLuint model, rot, view, projection;
		} loc;
		struct {
			GLuint model, rot, view, projection;
		} upd;
		QMatrix4x4 model, rot, view, projection;
		QVector<QVector3D> vertex, colour;
		QQuaternion quat;
	} data;
};

#endif // MOTIONGLWIDGET_H
