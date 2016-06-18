#ifndef VIEWWIDGET_H
#define VIEWWIDGET_H

#include <QtWidgets>
#include <QOpenGLFunctions_3_3_Core>
#include "report.h"

#define VIEW_GL_PROGRAM_COUNT	2

class ViewWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
	Q_OBJECT
public:
	explicit ViewWidget(QVector<report_t> *reports, QWidget *parent = 0);

protected:
	void initializeGL();
	void resizeGL(int w, int h);
	void resizeEvent(QResizeEvent *e);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);
	void paintGL();

private:
	void renderCursor();
	void renderReports();
	void updateCursor(int x);
	void updateCursorTimestamp(quint32 timestamp);
	quint32 cursorTimestamp();
	quint32 screenOffset();

	struct shader_t {
		QOpenGLShader::ShaderType type;
		const char *fileName;
	};

	bool addShaderFromSourceFile(QOpenGLShaderProgram &program,
				     QOpenGLShader::ShaderType type,
				     const QString &fileName);
	bool createPrograms();
	bool createProgram(QOpenGLShaderProgram &program, const QVector<shader_t> &shaders, const QString &name);
	bool linkProgram(QOpenGLShaderProgram &program, const QString &name);

	enum {
		PROGRAM_BASIC = 0,
		PROGRAM_PIXEL,
		PROGRAM_REPORT,
		PROGRAM_COUNT,
	};

	enum {
		ATTRIBUTE_POSITION = 0,
		ATTRIBUTE_COUNT,
	};

	static const QStringList programNames;
	static const QVector<shader_t> shaders[PROGRAM_COUNT];
	static const QStringList attributes;
	QOpenGLShaderProgram programs[PROGRAM_COUNT];

	struct {
		GLuint rect;
	} vao;

	struct {
		QMatrix4x4 projection, view;
	} matrix;

	static const QColor colours[16];

	struct cursor_t {
		cursor_t() : x(-1), scale(1000) {}
		bool isValid() {return x != -1;}

		QPoint mouse;
		int x;
		quint32 scale;
		quint32 timestamp;
	} cursor;
	QVector<report_t> *reports;
};

#endif // VIEWWIDGET_H
