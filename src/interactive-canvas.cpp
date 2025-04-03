#include <obs.h>
#include "interactive-canvas.h"
#include <qevent.h>
#include <qwindow.h>
#include <qthread.h>
#include "Processing.NDI.Lib.h"


ViscaAPI ptz_preview_get_visca_connection()
{
	return *_global_manager->getRecvInfo(_global_manager->getCurrent())
			.visca;
}
typedef struct context_t {
	obs_source_t *sourceToControl;
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
	bool drag_start;
	bool running;
	bool new_source;
	bool waiting_for_status;
	bool connected;
} context_t;

void draw_display(void *param, uint32_t cx, uint32_t cy)
{
	if (!param)
		return;

	auto context = (context_t *)param;

	if (context->sourceToControl == nullptr)
		return;

	gs_projection_push();
	gs_viewport_push();
	gs_set_viewport(0, 0, cx, cy);

	gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);

	// Set up scaling
	gs_matrix_push();
	gs_matrix_identity();

	// Get source dimensions
	uint32_t source_cx = obs_source_get_width(context->sourceToControl);
	uint32_t source_cy = obs_source_get_height(context->sourceToControl);

	if (source_cx && source_cy) {
		// Calculate scaling factors to fit the display
		float scale_x = (float)cx / (float)source_cx;
		float scale_y = (float)cy / (float)source_cy;

		// Use the smaller scale to maintain aspect ratio
		float scale = (scale_x < scale_y) ? scale_x : scale_y;
		gs_matrix_scale3f(scale, scale, 1.0f);
	}

	// Render the source with the applied scaling
	obs_source_video_render(context->sourceToControl);

	// Restore the previous matrices
	gs_matrix_pop();
	gs_viewport_pop();
	gs_projection_pop();
};

InteractiveCanvas::InteractiveCanvas(QWidget *parent, NDIPTZDeviceManager *manager) : QWidget(parent)
{
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_NativeWindow);
	_context = bzalloc(sizeof(context_t));
	_global_manager = manager;
}
	
InteractiveCanvas::~InteractiveCanvas()
{
	if (_display) {
		obs_display_remove_draw_callback(_display, draw_display,
						 (void *)this);
		obs_display_destroy(_display);
		_display = nullptr;
		if (((context_t *)_context)->sourceToControl)
			obs_source_release(
				((context_t *)_context)->sourceToControl);
		((context_t *)_context)->sourceToControl = nullptr;
	}
}

void InteractiveCanvas::initializeDisplay()
{
	if (_display)
		return;

	// Get the native window handle
	WId windowId = winId();

	// Setup display settings
	gs_init_data settings = {};
	settings.cx = width();
	settings.cy = height();
	settings.format = GS_RGBA;
	settings.zsformat = GS_ZS_NONE;

	// Pass your window handle as void pointer
#ifdef _WIN32
	settings.window.hwnd = (void *)windowId;
#elif defined(__APPLE__)
	settings.window.view = (void *)windowId;
#else // Linux
	settings.window.id = windowId;
	settings.window.display = QX11Info::display();
#endif

	// Create the display
	_display = obs_display_create(&settings, 0);
	obs_display_add_draw_callback(_display, draw_display, _context);
};

void InteractiveCanvas::mousePressEvent(QMouseEvent *event)
{
	QWidget::mousePressEvent(event);
	if (event->button() == Qt::LeftButton) {
		mouse_click(false, event->pos().x(), event->pos().y());
	}
}

void InteractiveCanvas::mouseMoveEvent(QMouseEvent *event)
{
	QWidget::mouseMoveEvent(event);
	mouse_move(event->buttons(), event->pos().x(), event->pos().y(), false);
}

void InteractiveCanvas::mouseReleaseEvent(QMouseEvent *event)
{
	QWidget::mouseReleaseEvent(event);
	if (event->button() == Qt::LeftButton) {
		mouse_click(true, event->pos().x(), event->pos().y());
	}
}
void InteractiveCanvas::enterEvent(QEnterEvent *event)
{
	QWidget::enterEvent(event);
	focus(true);
}

void InteractiveCanvas::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	if (_display) {
		obs_display_resize(_display, width(), height());
	}
}
void InteractiveCanvas::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
	if (!_display) {
		initializeDisplay();
	}
}
void InteractiveCanvas::setSource(obs_source_t *source)
{
	auto s = (context_t *)_context;
	auto old_source = s->sourceToControl;
	if (old_source)
		obs_source_release(old_source);
	s->sourceToControl = obs_source_get_ref(source);
	s->new_source = true;
	s->connected = false;
}
void InteractiveCanvas::set_wheel(int dx, int dy)
{
	auto s = (context_t *)_context;
	s->x_delta += dx;
	s->y_delta += dy;
	blog(LOG_INFO, "[patizo] ptz_controller_set_wheel [%d,%d]", s->x_delta,
	     s->y_delta);
}

void InteractiveCanvas::mouse_click(bool mouse_up, int x, int y)
{
	auto s = (context_t *)_context;
	s->mouse_down = !mouse_up;
	if (s->mouse_down) {
		s->x_start = x;
		s->y_start = y;
		s->x_move = x;
		s->y_move = y;
		s->drag_start = true;
	}
}

void InteractiveCanvas::mouse_move(int mods, int x, int y, bool mouse_leave)
{
	auto s = (context_t *)_context;
	if (s->mouse_down) {
		s->x_move = x;
		s->y_move = y;
	}
}
float pixels_to_pan_ratio(short zoom)
{
	return (1.0209e-09f * ((float)zoom * (float)zoom)) +
	       (-4.3145e-05f * (float)zoom) + 0.4452f;
};
float pixels_to_tilt_ratio(short zoom)
{
	return (-1.20079e-09f * ((float)zoom * (float)zoom)) +
	       (4.94849e-05f * (float)zoom) + -5.02822e-01f;
};
void InteractiveCanvas::focus(bool focus) {
	auto s = (context_t *)_context;	
	if (focus) {
		bool flip;
		visca_error_t errh =
			ptz_preview_get_visca_connection().getHorizontalFlip(
				flip);
		float h_flip = flip ? -1.f : 1.f;
		visca_error_t errv =
			ptz_preview_get_visca_connection().getVerticalFlip(
				flip);
		float v_flip = flip ? -1.f : 1.f;

		visca_error_t errz =
			ptz_preview_get_visca_connection().getZoomLevel(
				s->zoom); 

		s->pixels_to_pan = pixels_to_pan_ratio(s->zoom);
		s->pixels_to_tilt = pixels_to_tilt_ratio(s->zoom);

		blog(LOG_INFO,
		     "[patizo] ptz_controller_focus h=%f, v=%f, e=%d, zoom=%d, %f %f",
		     h_flip, v_flip, errz, s->zoom, s->pixels_to_pan,
		     s->pixels_to_tilt);
	}
};

class ControllerThread : public QThread {
public:
	ControllerThread(context_t *context) : _context(context) {}

protected:
	void run() override
	{	
		auto s = _context;
		blog(LOG_INFO, "[patizo] ptz_controller_thread_run");
		while (s->running) {
			if (_global_manager->getCurrent() != "") {
				if (s->y_delta != 0) {
					int newZoom = std::clamp(
						s->zoom + (s->y_delta * 4), 0,
						16384);
					blog(LOG_INFO,
					     "[patizo] ptz_controller zooming [%d] %d %d",
					     s->y_delta, s->zoom, newZoom);

					s->zoom = newZoom;
					auto err =
						ptz_preview_get_visca_connection()
							.setZoomLevel(newZoom);

					s->y_delta = 0;
				}
				if (s->drag_start) {
					auto err =
						ptz_preview_get_visca_connection()
							.getPanTilt(
								s->pt_start);
					auto errz =
						ptz_preview_get_visca_connection()
							.getZoomLevel(s->zoom);
					s->pixels_to_pan =
						pixels_to_pan_ratio(
							s->zoom);
					s->pixels_to_tilt =
						pixels_to_tilt_ratio(
							s->zoom);
					blog(LOG_INFO,
					     "[patizo] ptz_controller_mouse_click start drag err=%d, errz=%d, xy[%d,%d] pt[%d,%d]",
					     err, errz, s->x_start, s->y_start,
					     s->pt_start.value1,
					     s->pt_start.value2);
					s->drag_start = false;
				}
				if (s->mouse_down) {
					int dx = s->x_start - s->x_move;
					int dy = s->y_start - s->y_move;

					visca_tuple_t dest = {
						s->pt_start.value1 +
							(int)((float)dx *
							      s->pixels_to_pan),
						s->pt_start.value2 +
							(int)((float)dy *
							      s->pixels_to_tilt)};
					auto err =
						ptz_preview_get_visca_connection()
							.setAbsolutePanTilt(
								dest);
					blog(LOG_INFO,
					     "[patizo] ptz_controller_mouse_move xy[%d,%d] pt[%d,%d] px[%6.4f,%6.4f]",
					     s->x_start, s->y_start,
					     dest.value1, dest.value2,
					     s->pixels_to_pan,
					     s->pixels_to_tilt);
				}
				/*
				NDIlib_video_frame_v2_t video_frame;
				NDIlib_recv_instance_t recv =
					ptz_preview_get_recv();
				if (ptz_preview_get_ndilib()->recv_capture_v2(
					    recv, &video_frame, nullptr,
					    nullptr, 1000) ==
					    NDIlib_frame_type_status_change ) {
					{
					}
					// Free the frame
					ptz_preview_get_ndilib()
						->recv_free_video_v2(
							recv, &video_frame);
				}
				*/
			}
			std::this_thread::sleep_for(
				std::chrono::milliseconds(100));
		};
	}

private:
	context_t *_context;
};

void InteractiveCanvas::thread_start() {
	auto s = (context_t *)_context;
    s->running = true;
	s->new_source = false;
    s->waiting_for_status = false;
	s->connected = false;
    //_controllerThread = new ControllerThread(s); 
    //_controllerThread->start();
    blog(LOG_INFO, "[patizo] ptz_controller_thread_start");
}

void InteractiveCanvas::thread_stop() {
	auto s = (context_t *)_context;
    if (s->running) {
        s->running = false;
        //_controllerThread->quit();
        //_controllerThread->wait();
        //delete _controllerThread;
    }
    blog(LOG_INFO, "[patizo] ptz_controller_thread_stop");
}
