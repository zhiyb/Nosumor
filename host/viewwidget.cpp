#include "viewwidget.h"

#define CURSOR_COLOUR	QColor(0x66, 0xcc, 0xff)

extern volatile int fatal;

const QStringList ViewWidget::programNames = {
	"basic",
	"pixel",
	"report",
};

const QVector<ViewWidget::shader_t> ViewWidget::shaders[ViewWidget::PROGRAM_COUNT] = {
	{
		{QOpenGLShader::Vertex, ":/shaders/basic.vert"},
		{QOpenGLShader::Fragment, ":/shaders/basic.frag"},
	},{
		{QOpenGLShader::Vertex, ":/shaders/pixel.vert"},
		{QOpenGLShader::Fragment, ":/shaders/pixel.frag"},
	},{
		{QOpenGLShader::Vertex, ":/shaders/report.vert"},
		{QOpenGLShader::Fragment, ":/shaders/report.frag"},
	}
};

const QStringList ViewWidget::attributes = {
	"position"
};

const QColor ViewWidget::colours[16] = {
	Qt::red,	Qt::green,	Qt::blue,	Qt::white,
	Qt::yellow,	Qt::cyan,	Qt::magenta,	Qt::lightGray,
	Qt::red,	Qt::green,	Qt::blue,	Qt::white,
	Qt::yellow,	Qt::cyan,	Qt::magenta,	Qt::lightGray,
};

ViewWidget::ViewWidget(QVector<report_t> *reports, QWidget *parent) : QOpenGLWidget(parent)
{
	this->reports = reports;
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
	glEnable(GL_MULTISAMPLE);
	if (!createPrograms())
		goto failed;
	glClearColor(0.f, 0.f, 0.f, 1.f);

	glGenVertexArrays(1, &vao.rect);
	glBindVertexArray(vao.rect);

	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	const static GLfloat rectVertices[] = {0.f, 0.f,  0.f, 1.f,  1.f, 0.f,  1.f, 1.f,};
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTRIBUTE_POSITION);
	glVertexAttribPointer(ATTRIBUTE_POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);

	return;

failed:
	QCoreApplication::exit(1);
	fatal = 1;
	return;
}

void ViewWidget::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	matrix.projection.setToIdentity();
	matrix.projection.ortho(0.f, w, 0.f, 1.f, 1.f, -1.f);
}

void ViewWidget::resizeEvent(QResizeEvent *e)
{
	if (cursor.isValid()) {
		int ow = e->oldSize().width(), w = e->size().width();
		cursor.x = (cursor.x - ow / 2) * w / ow + w / 2;
		e->accept();
		update();
	}
	QOpenGLWidget::resizeEvent(e);
}

void ViewWidget::mousePressEvent(QMouseEvent *event)
{
	QOpenGLWidget::mousePressEvent(event);
	if (event->buttons() & Qt::RightButton)
		updateCursor(event->pos().x());
	cursor.mouse = event->pos();
	event->accept();
	update();
}

void ViewWidget::mouseMoveEvent(QMouseEvent *event)
{
	QOpenGLWidget::mouseMoveEvent(event);
	if (event->buttons() & Qt::LeftButton) {
		int move = cursor.mouse.x() - event->pos().x();
		updateCursorTimestamp(cursor.scale * move);
	}
	if (event->buttons() & Qt::RightButton)
		updateCursor(event->pos().x());
	cursor.mouse = event->pos();
	event->accept();
	update();
}

void ViewWidget::wheelEvent(QWheelEvent *event)
{
	if (event->delta() > 0)
		cursor.scale *= 2;
	else if (cursor.scale != 1)
		cursor.scale /= 2;
	update();
}

void ViewWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glBindVertexArray(vao.rect);
	renderReports();
	renderCursor();
}

void ViewWidget::renderCursor()
{
	if (!cursor.isValid())
		return;

	QOpenGLShaderProgram &program = programs[PROGRAM_PIXEL];
	program.bind();

	QMatrix4x4 mat;
	mat.translate(cursor.x - 1, 0.f, 0.f);
	mat.scale(3.f, 1.f, 1.f);
	mat = matrix.projection * mat;
	program.setUniformValue("mvpMatrix", mat);
	program.setUniformValue("colour", CURSOR_COLOUR);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void ViewWidget::renderReports()
{
	int size = reports->size();
	if (size < 2)
		return;
	QOpenGLShaderProgram &program = programs[PROGRAM_REPORT];
	program.bind();
	program.setUniformValue("size", this->size());
	quint32 offsetTimestamp = screenOffset();
	for (int i = 1; i != size; i++) {
		const report_t &prev = reports->at(i - 1);
		const report_t &report = reports->at(i);

		const int left = ((qint32)(prev.timestamp - offsetTimestamp)) / (qint32)cursor.scale;
		const int right = ((qint32)(report.timestamp - offsetTimestamp)) / (qint32)cursor.scale;
		const int size = right - left;
		QMatrix4x4 mat;
		mat.translate(left, 0.f, 0.f);
		mat.scale(size, 1.f);
		mat = matrix.projection * mat;
		program.setUniformValue("mvpMatrix", mat);

		program.setUniformValue("status", prev.status);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}

void ViewWidget::updateCursor(int x)
{
	if (reports->isEmpty()) {
		cursor.x = -1;
		return;
	}
	x = std::min(std::max(x, 0), width() - 1);
	quint32 offset = screenOffset();
	cursor.x = x;
	cursor.timestamp = offset + x * cursor.scale;
}

void ViewWidget::updateCursorTimestamp(quint32 timestamp)
{
	if (reports->isEmpty())
		return;
	else if (!cursor.isValid())
		updateCursor(width() / 2);
	cursor.timestamp += timestamp;
}

quint32 ViewWidget::cursorTimestamp()
{
	if (reports->isEmpty())
		return 0;
	else if (!cursor.isValid())
		return reports->last().timestamp;
	else
		return cursor.timestamp;
}

quint32 ViewWidget::screenOffset()
{
	if (reports->isEmpty())
		return 0;
	else if (!cursor.isValid())
		return reports->last().timestamp - cursor.scale * width();
	else
		return cursor.timestamp - cursor.scale * cursor.x;
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

bool ViewWidget::createPrograms()
{
	int i = 0;
	foreach (const QString &name, programNames) {
		if (!createProgram(programs[i], shaders[i], name))
			return false;
		i++;
	}
	return true;
}

bool ViewWidget::createProgram(QOpenGLShaderProgram &program, const QVector<shader_t> &shaders, const QString &name)
{
	foreach (const shader_t &shader, shaders)
		if (!addShaderFromSourceFile(program, shader.type, shader.fileName))
			return false;
	int i = 0;
	foreach (const QString &attribute, attributes)
		program.bindAttributeLocation(attribute, i++);
	return linkProgram(program, name);
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
