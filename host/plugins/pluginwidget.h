#ifndef PLUGINWIDGET_H
#define PLUGINWIDGET_H

#include <QWidget>

class PluginWidget : public QWidget
{
	Q_OBJECT
public:
	explicit PluginWidget(QWidget *parent = nullptr);

signals:
	void devRemove();

public slots:
};

#endif // PLUGINWIDGET_H
