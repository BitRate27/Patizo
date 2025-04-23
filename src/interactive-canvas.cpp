#include <obs.h>
#include "interactive-canvas.h"
#include <qevent.h>
#include <qwindow.h>
#include "Processing.NDI.Lib.h"

ViscaAPI ptz_preview_get_visca_connection()
{
	auto recv = _global_manager->getRecvInfo(_global_manager->getCurrent());
	if ((recv == nullptr) || (recv->visca == nullptr))
		return ViscaAPI();
	if (recv->visca->connectionStatus() != VOK) {
		// Try to reconnect
		recv->visca->reconnectCamera();
	}
	return *recv->visca;
}

CameraConfig OBSBOT_Tail_Air = {100, 400};

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

InteractiveCanvas::InteractiveCanvas(QWidget *parent,
				     NDIPTZDeviceManager *manager)
	: QWidget(parent)
{
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_NativeWindow);
	_context = bzalloc(sizeof(context_t));
	_global_manager = manager;
}

InteractiveCanvas::~InteractiveCanvas()
{
	if (_display) {
		thread_stop();
		obs_display_remove_draw_callback(_display, draw_display,
						 (void *)this);
		obs_display_destroy(_display);
		_display = nullptr;
		if (((context_t *)_context)->sourceToControl)
			obs_source_release(
				((context_t *)_context)->sourceToControl);
		((context_t *)_context)->sourceToControl = nullptr;
	}
	bfree(_context);
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
	thread_start();
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
void InteractiveCanvas::wheelEvent(QWheelEvent *event)
{
	// Get the delta (amount of scroll)
	int delta = event->angleDelta().y();
	set_wheel(0, delta); // 
	blog(LOG_INFO, "[patizo] Mouse wheel changed");

	// Accept the event
	event->accept();
}
void InteractiveCanvas::setSource(obs_source_t *source)
{
	auto s = (context_t *)_context;
	auto old_source = s->sourceToControl;
	if (old_source)
		obs_source_release(old_source);
	s->sourceToControl = obs_source_get_ref(source);

	auto source_name = obs_source_get_name(source);
	if (source_name != "") {
		s->receiver = _global_manager->getRecvInfo(source_name);
	}
	s->new_source = true;
	s->camera_config = OBSBOT_Tail_Air;
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
void InteractiveCanvas::focus(bool focus)
{
	auto s = (context_t *)_context;
	if (s->receiver == nullptr)
		return;
	auto vc = s->receiver->visca;
	if (vc == nullptr) {
		return;
	}
	if (focus) {
		bool flip;
		visca_error_t errh = vc->getHorizontalFlip(flip);
		s->x_flip = flip ? 1.f : -1.f;
		visca_error_t errv = vc->getVerticalFlip(flip);
		s->y_flip = flip ? 1.f : -1.f;

		visca_error_t errz = vc->getZoomLevel(s->zoom);

		s->pixels_to_pan = pixels_to_pan_ratio(s->zoom);
		s->pixels_to_tilt = pixels_to_tilt_ratio(s->zoom);

		blog(LOG_INFO,
		     "[patizo] ptz_controller_focus h=%f, v=%f, e=%d, zoom=%d, %f %f",
		     s->x_flip, s->y_flip, errz, s->zoom, s->pixels_to_pan,
		     s->pixels_to_tilt);
	}
};

void *controller_thread(void *data) {
	auto s = (context_t *)data;
	blog(LOG_INFO, "[patizo] ptz_controller_thread started");
	while (s->running) {
		if (s->receiver->visca != nullptr) {
			auto vc = s->receiver->visca;
			if (s->y_delta != 0) {
				int newZoom = std::clamp(
					s->zoom + (s->y_delta / 4), 100,
					400);
				blog(LOG_INFO,
					    "[patizo] ptz_controller zooming [%d] %d %d",
					    s->y_delta, s->zoom, newZoom);

				s->zoom = newZoom;
				auto err = vc->setZoomLevel(newZoom);
				blog(LOG_INFO,
				     "[patizo] ptz_controller zoomed [%d]",
				     err);
				s->y_delta = 0;
			}
			if (s->drag_start) {
				auto err = vc->getPanTilt(s->pt_start);
				auto errz = vc->getZoomLevel(s->zoom);
				s->pixels_to_pan =
					pixels_to_pan_ratio(s->zoom);
				s->pixels_to_tilt =
					pixels_to_tilt_ratio(s->zoom);
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
							    s->pixels_to_pan * s->x_flip),
					s->pt_start.value2 +
						(int)((float)dy *
							    s->pixels_to_tilt * s->y_flip)};
				auto err = vc->setAbsolutePanTilt(dest);
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
	blog(LOG_INFO, "[patizo] ptz_controller_thread exitted");
	return nullptr;
}

void InteractiveCanvas::thread_start()
{
	auto s = (context_t *)_context;
	pthread_create(&_controllerThread, nullptr, controller_thread, s);
	s->running = true;
	s->new_source = false;
	s->waiting_for_status = false;
	s->connected = false;
	blog(LOG_INFO, "[patizo] ptz_controller_thread_start");
}

void InteractiveCanvas::thread_stop()
{
	auto s = (context_t *)_context;
	if (s->running) {
		s->running = false;
		pthread_join(_controllerThread, NULL);	
		blog(LOG_INFO, "[patizo] ptz_controller_thread_stop");
	}

}
