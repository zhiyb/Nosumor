#include <QOpenGLFramebufferObject>
#include <QtWidgets>
#include "motionglwidget.h"

MotionGLWidget::MotionGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
	setMinimumSize(160, 160);
	setAutoFillBackground(false);

	data.model.setToIdentity();
	data.model.rotate(120, 1.0, 1.0, 1.0);
	data.model.scale(0.6, 0.2, 0.6);
	data.rot.setToIdentity();
	data.view.setToIdentity();
	data.view.rotate(-90, 0.0, 1.0, 0.0);
	data.view.rotate(-90, 1.0, 0.0, 0.0);
	data.projection.setToIdentity();
	data.vertex = QVector<QVector3D>({
	// Front
	QVector3D(1.0, 1.0, 1.0), QVector3D(-1.0, 1.0, 1.0), QVector3D(-1.0, -1.0, 1.0),
	QVector3D(-1.0, -1.0, 1.0), QVector3D(1.0, -1.0, 1.0), QVector3D(1.0, 1.0, 1.0),
	// Left
	QVector3D(-1.0, -1.0, 1.0), QVector3D(-1.0, 1.0, 1.0), QVector3D(-1.0, 1.0, -1.0),
	QVector3D(-1.0, 1.0, -1.0), QVector3D(-1.0, -1.0, -1.0), QVector3D(-1.0, -1.0, 1.0),
	// Back
	QVector3D(-1.0, 1.0, -1.0), QVector3D(1.0, 1.0, -1.0), QVector3D(1.0, -1.0, -1.0),
	QVector3D(1.0, -1.0, -1.0), QVector3D(-1.0, -1.0, -1.0), QVector3D(-1.0, 1.0, -1.0),
	// Right
	QVector3D(1.0, -1.0, -1.0), QVector3D(1.0, 1.0, -1.0), QVector3D(1.0, 1.0, 1.0),
	QVector3D(1.0, 1.0, 1.0), QVector3D(1.0, -1.0, 1.0), QVector3D(1.0, -1.0, -1.0),
	// Top
	QVector3D(1.0, 1.0, 1.0), QVector3D(1.0, 1.0, -1.0), QVector3D(-1.0, 1.0, -1.0),
	QVector3D(-1.0, 1.0, -1.0), QVector3D(-1.0, 1.0, 1.0), QVector3D(1.0, 1.0, 1.0),
	// Bottom
	QVector3D(-1.0, -1.0, -1.0), QVector3D(1.0, -1.0, -1.0), QVector3D(1.0, -1.0, 1.0),
	QVector3D(1.0, -1.0, 1.0), QVector3D(-1.0, -1.0, 1.0), QVector3D(-1.0, -1.0, -1.0),
	});
	data.colour = QVector<QVector3D>({
	// Front
	QVector3D(1.0, 0.0, 0.0), QVector3D(1.0, 0.0, 0.0), QVector3D(1.0, 0.0, 0.0),
	QVector3D(1.0, 0.0, 0.0), QVector3D(1.0, 0.0, 0.0), QVector3D(1.0, 0.0, 0.0),
	// Left
	QVector3D(0.0, 1.0, 0.0), QVector3D(0.0, 1.0, 0.0), QVector3D(0.0, 1.0, 0.0),
	QVector3D(0.0, 1.0, 0.0), QVector3D(0.0, 1.0, 0.0), QVector3D(0.0, 1.0, 0.0),
	// Back
	QVector3D(0.0, 1.0, 1.0), QVector3D(0.0, 1.0, 1.0), QVector3D(0.0, 1.0, 1.0),
	QVector3D(0.0, 1.0, 1.0), QVector3D(0.0, 1.0, 1.0), QVector3D(0.0, 1.0, 1.0),
	// Right
	QVector3D(1.0, 0.0, 1.0), QVector3D(1.0, 0.0, 1.0), QVector3D(1.0, 0.0, 1.0),
	QVector3D(1.0, 0.0, 1.0), QVector3D(1.0, 0.0, 1.0), QVector3D(1.0, 0.0, 1.0),
	// Top
	QVector3D(0.0, 0.0, 1.0), QVector3D(0.0, 0.0, 1.0), QVector3D(0.0, 0.0, 1.0),
	QVector3D(0.0, 0.0, 1.0), QVector3D(0.0, 0.0, 1.0), QVector3D(0.0, 0.0, 1.0),
	// Bottom
	QVector3D(1.0, 1.0, 0.0), QVector3D(1.0, 1.0, 0.0), QVector3D(1.0, 1.0, 0.0),
	QVector3D(1.0, 1.0, 0.0), QVector3D(1.0, 1.0, 0.0), QVector3D(1.0, 1.0, 0.0),
	});
}

void MotionGLWidget::updateQuaternion(int32_t *q)
{
	const float scale = (float)(1 << 30);
	data.quat = QQuaternion((float)q[0] / scale, (float)q[1] / scale,
			(float)q[2] / scale, (float)q[3] / scale);
	data.rot = QMatrix4x4(data.quat.toRotationMatrix());
	update();
}

void MotionGLWidget::initializeGL()
{
	initializeOpenGLFunctions();
	glEnable(GL_CULL_FACE);
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
				"void main(void)\n"
				"{\n"
				"    colour = vec4(vf_colour, 1.0);\n"
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
	data.loc.model = glGetUniformLocation(program, "model");
	data.loc.rot = glGetUniformLocation(program, "rot");
	data.loc.view = glGetUniformLocation(program, "view");
	data.loc.projection = glGetUniformLocation(program, "projection");
	glUseProgram(program);

	glEnableVertexAttribArray(data.loc.vertex);
	glVertexAttribPointer(data.loc.vertex, 3, GL_FLOAT, GL_FALSE, 0,
			      data.vertex.constData());
	glEnableVertexAttribArray(data.loc.colour);
	glVertexAttribPointer(data.loc.colour, 3, GL_FLOAT, GL_FALSE, 0,
			      data.colour.constData());
	glUniformMatrix4fv(data.loc.model, 1, GL_FALSE,
			   data.model.constData());
	glUniformMatrix4fv(data.loc.rot, 1, GL_FALSE,
			   data.rot.constData());
	glUniformMatrix4fv(data.loc.view, 1, GL_FALSE,
			   data.view.constData());
	glUniformMatrix4fv(data.loc.projection, 1, GL_FALSE,
			   data.projection.constData());
}

void MotionGLWidget::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	float asp = (float)w / (float)h;
	data.projection.setToIdentity();
	data.projection.ortho(-asp, asp, -1, 1, -1, 1);
	glUniformMatrix4fv(data.loc.projection, 1, GL_FALSE,
			   data.projection.constData());
}

void MotionGLWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(data.program);
	glUniformMatrix4fv(data.loc.rot, 1, GL_FALSE,
			   data.rot.constData());
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
