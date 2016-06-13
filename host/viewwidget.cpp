#include "viewwidget.h"

extern volatile int fatal;

ViewWidget::ViewWidget(QWidget *parent) : QOpenGLWidget(parent)
{
	setMinimumSize(100, 100);
}

void ViewWidget::initializeGL()
{
	if (format().version() != QPair<int, int>(3, 3)) {
		qDebug() << format();
		QMessageBox::critical(this, tr("OpenGL error"), tr("OpenGL version 3.3 not available"));
		goto failed;
	}

	initializeOpenGLFunctions();
	glClearColor(0.f, 0.f, 0.f, 1.f);

	if (!addShaderFromSourceFile(program.basic, QOpenGLShader::Vertex, ":/shaders/basic.vert"))
		goto failed;
	if (!addShaderFromSourceFile(program.basic, QOpenGLShader::Fragment, ":/shaders/basic.frag"))
		goto failed;
	if (!linkProgram(program.basic, "basic"))
		goto failed;
	return;

failed:
	QCoreApplication::exit(1);
	fatal = 1;
	return;
}

void ViewWidget::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	matrix.projection.ortho(0, w, h, 0, 1, -1);
}

void ViewWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT);
}

bool ViewWidget::addShaderFromSourceFile(QOpenGLShaderProgram &program, QOpenGLShader::ShaderType type, const QString &fileName)
{
	if (!program.addShaderFromSourceFile(type, fileName)) {
		qDebug() << "Failing shader:" << fileName;
		QMessageBox::critical(this, tr("OpenGL error"), tr("Shader %1 compile error:\n%2").arg(fileName).arg(program.log()));
		return false;
	}
	return true;
}

bool ViewWidget::linkProgram(QOpenGLShaderProgram &program, const QString &name)
{
	if (!program.link()) {
		qDebug() << "Failing program:" << name;
		QMessageBox::critical(this, tr("OpenGL error"), tr("Program %1 linking failed:\n%2").arg(name).arg(program.log()));
		return false;
	}
	return true;
}
