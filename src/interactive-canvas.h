#pragma once
#include <obs.h>
#include <qwidget.h>
#include <qthread.h>
#include <qevent.h>
#include "ndi-ptz-device-manager.h"

static NDIPTZDeviceManager *_global_manager;

typedef struct context_t {
	obs_source_t *sourceToControl;
	Receiver *receiver;
	short x_delta;
	short y_delta;
	short x_start;
	short y_start;
	short x_move;
	short y_move;
	int zoom;
	float pixels_to_pan;
	float pixels_to_tilt;
	visca_tuple_t pt_start;
	bool mouse_down;
	float x_flip;
	float y_flip;
	bool drag_start;
	bool running;
	bool new_source;
	bool waiting_for_status;
	bool connected;
} context_t;

class ControllerThread : public QThread {
public:
	ControllerThread(context_t *context) : _context(context) {}
protected:
	void run() override;
private:
	context_t *_context;
};
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
	void wheelEvent(QWheelEvent *event) override;

private:
	obs_display_t *_display = nullptr;
	ControllerThread *_controllerThread = nullptr;
	void *_context = nullptr;
	void initializeDisplay();
	void set_wheel(int dx, int dy);
	void mouse_click(bool mouse_up, int x, int y);
	void mouse_move(int mods, int x, int y, bool mouse_leave);
	void focus(bool focus);
	void thread_start();
	void thread_stop();
};
