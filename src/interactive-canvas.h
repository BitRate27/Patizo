#pragma once
#include <obs.h>
#include <qwidget.h>
#include <qthread.h>
#include <qevent.h>
#include "ndi-ptz-device-manager.h"
static NDIPTZDeviceManager *_global_manager;

class InteractiveCanvas : public QWidget {
public:
	InteractiveCanvas(QWidget *parent, NDIPTZDeviceManager *manager);
	~InteractiveCanvas();
	void setSource(obs_source_t *source);

protected:

	void resizeEvent(QResizeEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void enterEvent(QEnterEvent *event) override;

private:
	obs_display_t *_display = nullptr;
	//ControllerThread *_controllerThread = nullptr;
	void *_context = nullptr;
	void initializeDisplay();
	void set_wheel(int dx, int dy);
	void mouse_click(bool mouse_up, int x, int y);
	void mouse_move(int mods, int x, int y, bool mouse_leave);
	void focus(bool focus);
	void thread_start();
	void thread_stop();
};