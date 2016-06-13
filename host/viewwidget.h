#ifndef VIEWWIDGET_H
#define VIEWWIDGET_H

#include <QtWidgets>
#include <QOpenGLFunctions_3_3_Core>

class ViewWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
	Q_OBJECT
public:
	explicit ViewWidget(QWidget *parent = 0);

protected:
	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();

private:
	bool addShaderFromSourceFile(QOpenGLShaderProgram &program,
				     QOpenGLShader::ShaderType type,
				     const QString &fileName);
	bool linkProgram(QOpenGLShaderProgram &program, const QString &name);

	struct {
		QMatrix4x4 projection, view;
	} matrix;

	struct {
		QOpenGLShaderProgram basic;
	} program;
};

#endif // VIEWWIDGET_H
