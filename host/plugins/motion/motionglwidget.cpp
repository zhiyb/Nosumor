#include <QOpenGLFramebufferObject>
#include <QtWidgets>
#include "motionglwidget.h"

MotionGLWidget::MotionGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
	setMinimumSize(320, 160);
	setAutoFillBackground(false);

	data.model.setToIdentity();
	data.model.scale(0.6, 0.2, 0.6);

	data.compass.model.setToIdentity();
	data.compass.model.scale(0.75, 0.0125, 0.0125);
	data.compass.model.translate(1.0, 1.0, 1.0);

	data.rot.setToIdentity();
	data.view.setToIdentity();
	data.view.rotate(-90, 0.0, 1.0, 0.0);
	data.view.rotate(-90, 1.0, 0.0, 0.0);
	data.upd.view = 1;
	data.projection[0].setToIdentity();
	data.projection[1].setToIdentity();

	data.vertex = QVector<QVector3D>({
	// Right
	QVector3D(1.0, -1.0, -1.0), QVector3D(1.0, 1.0, -1.0), QVector3D(1.0, 1.0, 1.0),
	QVector3D(1.0, 1.0, 1.0), QVector3D(1.0, -1.0, 1.0), QVector3D(1.0, -1.0, -1.0),
	// Top
	QVector3D(1.0, 1.0, 1.0), QVector3D(1.0, 1.0, -1.0), QVector3D(-1.0, 1.0, -1.0),
	QVector3D(-1.0, 1.0, -1.0), QVector3D(-1.0, 1.0, 1.0), QVector3D(1.0, 1.0, 1.0),
	// Front
	QVector3D(1.0, 1.0, 1.0), QVector3D(-1.0, 1.0, 1.0), QVector3D(-1.0, -1.0, 1.0),
	QVector3D(-1.0, -1.0, 1.0), QVector3D(1.0, -1.0, 1.0), QVector3D(1.0, 1.0, 1.0),
	// Left
	QVector3D(-1.0, -1.0, 1.0), QVector3D(-1.0, 1.0, 1.0), QVector3D(-1.0, 1.0, -1.0),
	QVector3D(-1.0, 1.0, -1.0), QVector3D(-1.0, -1.0, -1.0), QVector3D(-1.0, -1.0, 1.0),
	// Bottom
	QVector3D(-1.0, -1.0, -1.0), QVector3D(1.0, -1.0, -1.0), QVector3D(1.0, -1.0, 1.0),
	QVector3D(1.0, -1.0, 1.0), QVector3D(-1.0, -1.0, 1.0), QVector3D(-1.0, -1.0, -1.0),
	// Back
	QVector3D(-1.0, 1.0, -1.0), QVector3D(1.0, 1.0, -1.0), QVector3D(1.0, -1.0, -1.0),
	QVector3D(1.0, -1.0, -1.0), QVector3D(-1.0, -1.0, -1.0), QVector3D(-1.0, 1.0, -1.0),
	});
	data.colour = QVector<QVector3D>({
	// Right
	QVector3D(1.0, 0.0, 0.0), QVector3D(1.0, 0.0, 0.0), QVector3D(1.0, 0.0, 0.0),
	QVector3D(1.0, 0.0, 0.0), QVector3D(1.0, 0.0, 0.0), QVector3D(1.0, 0.0, 0.0),
	// Top
	QVector3D(0.0, 1.0, 0.0), QVector3D(0.0, 1.0, 0.0), QVector3D(0.0, 1.0, 0.0),
	QVector3D(0.0, 1.0, 0.0), QVector3D(0.0, 1.0, 0.0), QVector3D(0.0, 1.0, 0.0),
	// Front
	QVector3D(0.0, 0.0, 1.0), QVector3D(0.0, 0.0, 1.0), QVector3D(0.0, 0.0, 1.0),
	QVector3D(0.0, 0.0, 1.0), QVector3D(0.0, 0.0, 1.0), QVector3D(0.0, 0.0, 1.0),
	// Left
	QVector3D(0.0, 1.0, 1.0), QVector3D(0.0, 1.0, 1.0), QVector3D(0.0, 1.0, 1.0),
	QVector3D(0.0, 1.0, 1.0), QVector3D(0.0, 1.0, 1.0), QVector3D(0.0, 1.0, 1.0),
	// Bottom
	QVector3D(1.0, 0.0, 1.0), QVector3D(1.0, 0.0, 1.0), QVector3D(1.0, 0.0, 1.0),
	QVector3D(1.0, 0.0, 1.0), QVector3D(1.0, 0.0, 1.0), QVector3D(1.0, 0.0, 1.0),
	// Back
	QVector3D(1.0, 1.0, 0.0), QVector3D(1.0, 1.0, 0.0), QVector3D(1.0, 1.0, 0.0),
	QVector3D(1.0, 1.0, 0.0), QVector3D(1.0, 1.0, 0.0), QVector3D(1.0, 1.0, 0.0),
	});
	data.compass.colour = QVector<QVector3D>(data.colour.size(),
						 QVector3D(1.0, 1.0, 1.0));
}

void MotionGLWidget::updateQuaternion(int32_t *p)
{
	const float scale = (float)(1 << 30);
	data.quat = QQuaternion((float)p[0] / scale, (float)p[1] / scale,
			(float)p[2] / scale, (float)p[3] / scale);
	data.rot = QMatrix4x4(data.quat.toRotationMatrix());
	update();
}

void MotionGLWidget::updateCompass(int16_t *p)
{
	const float scale = (float)1.0;
	QVector3D v((float)p[0] / scale, (float)p[1] / scale,
			(float)p[2] / scale);
	v.normalize();
	QVector3D m(1.0, 0.0, 0.0);
	QVector3D n(QVector3D::crossProduct(m, v));
	float a = acos(QVector3D::dotProduct(m, v)) * 180.0 / M_PI;
	data.compass.rot.setToIdentity();
	data.compass.rot.rotate(a, n);
	update();
}

void MotionGLWidget::reset()
{
	QVector3D v(1.0, 0.0, 0.0);
	QVector3D r = data.view * data.rot * data.model * -v;
	r.normalize();
	QVector3D n = QVector3D::crossProduct(r, v);
	float a = acos(QVector3D::dotProduct(r, v));
	QMatrix4x4 rot;
	rot.rotate(a * 180.0 / M_PI, n);
	data.view = rot * data.view;

	v = QVector3D(0.0, 1.0, 0.0);
	r = data.view * data.rot * data.model * v;
	r.normalize();
	n = QVector3D::crossProduct(r, v);
	a = acos(QVector3D::dotProduct(r, v));
	rot.setToIdentity();
	rot.rotate(a * 180.0 / M_PI, n);
	data.view = rot * data.view;

	data.upd.view = 1;
	update();
}

void MotionGLWidget::initializeGL()
{
	initializeOpenGLFunctions();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 1.0);

	GLuint program = glCreateProgram();
	GLuint vsh = loadShader(GL_VERTEX_SHADER,
				"#version 130\n"
				"in vec3 vertex;\n"
				"in vec3 colour;\n"
				"uniform mat4 model, rot, view, projection;\n"
				"out vec3 vf_colour;\n"
				"void main(void)\n"
				"{\n"
				"    gl_Position = projection * view * rot * model * vec4(vertex, 1.);\n"
				"    vf_colour = colour;\n"
				"}\n");
	if (!vsh) {
		deleteLater();
		return;
	}
	glAttachShader(program, vsh);
	GLuint fsh = loadShader(GL_FRAGMENT_SHADER,
				"#version 130\n"
				"in vec3 vf_colour;\n"
				"out vec4 colour;\n"
				"uniform float alpha;\n"
				"void main(void)\n"
				"{\n"
				"    colour = vec4(vf_colour, alpha);\n"
				"}\n");
	if (!fsh) {
		deleteLater();
		return;
	}
	glAttachShader(program, fsh);

	glLinkProgram(program);
	int logLength;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	char log[logLength] = {0};
	glGetProgramInfoLog(program, logLength, &logLength, log);
	if (logLength)
		qWarning(log);
	if (!program) {
		deleteLater();
		return;
	}
	data.program = program;

	data.loc.vertex = glGetAttribLocation(program, "vertex");
	data.loc.colour = glGetAttribLocation(program, "colour");
	data.loc.alpha = glGetUniformLocation(program, "alpha");
	data.loc.model = glGetUniformLocation(program, "model");
	data.loc.rot = glGetUniformLocation(program, "rot");
	data.loc.view = glGetUniformLocation(program, "view");
	data.loc.projection = glGetUniformLocation(program, "projection");
	glUseProgram(program);

	glEnableVertexAttribArray(data.loc.vertex);
	glVertexAttribPointer(data.loc.vertex, 3, GL_FLOAT, GL_FALSE, 0,
			      data.vertex.constData());
	glEnableVertexAttribArray(data.loc.colour);
}

void MotionGLWidget::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	float asp = (float)w / (float)h;
	data.projection[0].setToIdentity();
	data.projection[0].translate(-0.5, 0.0, 0.0);
	data.projection[0].ortho(-asp, asp, -1, 1, -1, 1);
	data.projection[1].setToIdentity();
	data.projection[1].translate(0.5, 0.0, 0.0);
	data.projection[1].ortho(-asp, asp, -1, 1, -1, 1);
	data.projection[1].rotate(90, 1.0, 0.0, 0.0);
}

void MotionGLWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(data.program);
	if (data.upd.view) {
		glUniformMatrix4fv(data.loc.view, 1, GL_FALSE,
				   data.view.constData());
		data.upd.view = 0;
	}
	render(0);
	render(1);
}

void MotionGLWidget::render(int pj)
{
	// Render compass
	data.compass.rv = data.rot * data.compass.rot;
	glUniformMatrix4fv(data.loc.model, 1, GL_FALSE,
			   data.compass.model.constData());
	glUniformMatrix4fv(data.loc.rot, 1, GL_FALSE,
			   data.compass.rv.constData());
	glUniformMatrix4fv(data.loc.projection, 1, GL_FALSE,
			   data.projection[pj].constData());
	glUniform1f(data.loc.alpha, 1.0);
	glVertexAttribPointer(data.loc.colour, 3, GL_FLOAT, GL_FALSE, 0,
			      data.compass.colour.constData());
	glDrawArrays(GL_TRIANGLES, 0, data.vertex.size());
	// Render box
	glUniformMatrix4fv(data.loc.model, 1, GL_FALSE,
			   data.model.constData());
	glUniformMatrix4fv(data.loc.rot, 1, GL_FALSE,
			   data.rot.constData());
	glUniformMatrix4fv(data.loc.projection, 1, GL_FALSE,
			   data.projection[pj].constData());
	glUniform1f(data.loc.alpha, 0.75);
	glVertexAttribPointer(data.loc.colour, 3, GL_FLOAT, GL_FALSE, 0,
			      data.colour.constData());
	glDrawArrays(GL_TRIANGLES, 0, data.vertex.size());
}

GLuint MotionGLWidget::loadShader(GLenum type, const QByteArray& context)
{
	GLuint shader = glCreateShader(type);
	const char *p = context.constData();
	int length = context.length();
	glShaderSource(shader, 1, &p, &length);
	glCompileShader(shader);

	int status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	int logLength;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	char log[logLength];
	glGetShaderInfoLog(shader, logLength, &logLength, log);
	if (logLength)
		qWarning(log);
	if (status == GL_TRUE)
		return shader;
	glDeleteShader(shader);
	return 0;
}
