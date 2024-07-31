#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>

#include <qevent.h>
#include <qpainter.h>
#include <qlabel.h>
#include <qlayout.h>
#include <chrono>
#include <pthread.h>
#include <thread>
#include <regex>
#include <mutex>
#include "ptz-controller.h"
#include "ndi-ptz-device-manager.h"
#include <Processing.NDI.Lib.h>

// Function to convert a single UYVY pixel pair to two RGB pixels
void UYVYtoRGB(int U, int Y1, int V, int Y2, QRgb &rgb1, QRgb &rgb2)
{
	int C1 = Y1 - 16;
	int C2 = Y2 - 16;
	int D = U - 128;
	int E = V - 128;

	int R1 = (298 * C1 + 409 * E + 128) >> 8;
	int G1 = (298 * C1 - 100 * D - 208 * E + 128) >> 8;
	int B1 = (298 * C1 + 516 * D + 128) >> 8;

	int R2 = (298 * C2 + 409 * E + 128) >> 8;
	int G2 = (298 * C2 - 100 * D - 208 * E + 128) >> 8;
	int B2 = (298 * C2 + 516 * D + 128) >> 8;

	// Clamp the values to 0-255
	rgb1 = qRgb(qBound(0, R1, 255), qBound(0, G1, 255), qBound(0, B1, 255));
	rgb2 = qRgb(qBound(0, R2, 255), qBound(0, G2, 255), qBound(0, B2, 255));
}

// Main conversion function
void convertUYVYtoRGB32(const unsigned char *uyvyData,
					     int width, int height, unsigned char *rgbaData)
{
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; x += 2) {
			int index = (y * width + x) *
				    2; 
			int U = uyvyData[index];
			int Y1 = uyvyData[index + 1];
			int V = uyvyData[index + 2];
			int Y2 = uyvyData[index + 3];

			QRgb rgb1, rgb2;
			UYVYtoRGB(U, Y1, V, Y2, rgb1, rgb2);

			int r1Index = (y * width + x) * 4;
			unsigned int *r1ptr = (unsigned int *)&rgbaData[r1Index];
			*r1ptr = rgb1;

			if (x + 1 < width) { // Check to avoid buffer overflow
				unsigned int *r2ptr =
					(unsigned int *)&rgbaData[r1Index+4];
				*r2ptr = rgb2;
			}
		}
	}
}
void debugRGB32(const unsigned char *abgrData, int width, int height, unsigned char *rgbaData)
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int index = (y * (width*4) + (x*4));
			rgbaData[index + 3] = abgrData[index + 0];
			rgbaData[index + 2] = abgrData[index + 3];
			rgbaData[index + 1] = abgrData[index + 2];
			rgbaData[index + 0] = abgrData[index + 1];
		}
	}
}
void newconvertUYVYtoRGB32(const unsigned char *uyvy, int width, int height,
			unsigned char *rgb)
{
	for (int i = 0; i < width * height / 2; ++i) {
		int u = uyvy[i * 4 + 0];
		int y1 = uyvy[i * 4 + 1];
		int v = uyvy[i * 4 + 2];
		int y2 = uyvy[i * 4 + 3];

		int c = y1 - 16;
		int d = u - 128;
		int e = v - 128;

		rgb[i * 8 + 0] =
			(unsigned char)qBound(0, (298 * c + 409 * e + 128) >> 8, 255); // r
		rgb[i * 8 + 1] = (unsigned char)qBound(
			0, (298 * c - 100 * d - 208 * e + 128) >> 8, 255); // g
		rgb[i * 8 + 2] =
			(unsigned char)qBound(0, (298 * c + 516 * d + 128) >> 8, 255); // b
		rgb[i * 8 + 3] = 255;                                   // a

		c = y2 - 16;

		rgb[i * 8 + 4] =
			(unsigned char)qBound(0, (298 * c + 409 * e + 128) >> 8, 255); // r
		rgb[i * 8 + 5] = (unsigned char)qBound(
			0, (298 * c - 100 * d - 208 * e + 128) >> 8, 255); // g
		rgb[i * 8 + 6] =
			(unsigned char)qBound(0, (298 * c + 516 * d + 128) >> 8, 255); // b
		rgb[i * 8 + 7] = 255;                                   // a
	}
}
InteractiveCanvas::InteractiveCanvas(QWidget *parent,
				     const NDIlib_v4 *ndiLib,
				     NDIPTZDeviceManager *manager)
	: QWidget(parent),
	  _ndiLib(ndiLib),
	  _manager(manager)
{		
}

void InteractiveCanvas::updateImage(int dummy)
{
	(void)dummy;
	update();
}

void InteractiveCanvas::mousePressEvent(QMouseEvent *event) 
{
	if (event->button() == Qt::LeftButton) {
	}
}

void InteractiveCanvas::mouseMoveEvent(QMouseEvent *event)
{
	if ((event->buttons() & Qt::LeftButton))
	{
	}
}

void InteractiveCanvas::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
	}
}

QRect InteractiveCanvas::imageRect(QImage &image) const {
	QSize scaledSize =
		image.size().scaled(this->size(), Qt::KeepAspectRatio);
	// Calculate the top left position to center the image
	QPoint topLeft((this->width() - scaledSize.width()) / 2,
		       (this->height() - scaledSize.height()) / 2);
	return QRect(topLeft, scaledSize);
};
void InteractiveCanvas::paintEvent(QPaintEvent *event)
{
	(void)event;
	QPainter painter(this);
	NDIlib_video_frame_v2_t video_frame;
	auto recv = _manager->getRecvInfo(_manager->getCurrent()).recv;
	NDIlib_frame_type_e frame_type = _ndiLib->recv_capture_v2(
		recv,
		&video_frame, nullptr, nullptr, 1000);

	if (frame_type == NDIlib_frame_type_video) {
		QVector<uchar> rgbData(video_frame.xres * video_frame.yres * 4);

		// Convert UYVY to RGB32
		//newconvertUYVYtoRGB32(video_frame.p_data, video_frame.xres,
		//		   video_frame.yres, rgbData.data());

		// Create QImage from the converted data
		debugRGB32(video_frame.p_data, video_frame.xres,
			   video_frame.yres, rgbData.data());
		QImage image(rgbData.data(), video_frame.xres, video_frame.yres,
			     QImage::Format_RGBA8888);

		if (!image.isNull()) {
			// Draw the image
			painter.drawImage(imageRect(image), image);
		}

		// Free the NDI video frame
		_ndiLib->recv_free_video_v2(recv, &video_frame);
	}
}

void InteractiveCanvas::resizeEvent(QResizeEvent *event)
{
	if (width() > canvas.width() || height() > canvas.height()) {
		int newWidth = qMax(width() + 128, canvas.width());
		int newHeight = qMax(height() + 128, canvas.height());
		resizeCanvas(QSize(newWidth, newHeight));
		update();
	}
	QWidget::resizeEvent(event);
}

void InteractiveCanvas::resizeCanvas(const QSize &newSize)
{
	if (canvas.size() == newSize)
		return;

	QImage newCanvas(newSize, QImage::Format_RGB32);
	newCanvas.fill(Qt::white);
	QPainter painter(&newCanvas);
	painter.drawImage(imageRect(canvas), canvas);
	canvas = newCanvas;
}


class PTZControllerWidget : public QWidget {

public:
	PTZControllerWidget(const NDIlib_v4* ndiLib, NDIPTZDeviceManager* manager) : 
		_ndiLib(ndiLib), _manager(manager)
	{
		_label = new QLabel("");
		_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		QVBoxLayout *vbox = new QVBoxLayout(this);
		vbox->addWidget(_label);

		canvas = new InteractiveCanvas(this,_ndiLib,_manager);
		canvas->setSizePolicy(QSizePolicy::Expanding,
				      QSizePolicy::Expanding);
		vbox->addWidget(canvas);
		canvas->show();

    	blog(LOG_INFO, "[obs-ndi] ptz_controller_thread_start");
	};

	void resizeEvent(QResizeEvent *event) override 
	{ 
		QWidget::resizeEvent(event);
		canvas->resize(this->size());
	};
	InteractiveCanvas *canvas;

protected:
	void paintEvent(QPaintEvent *event) override
	{
		(void)event;
		_label->setText(_manager->getCurrent().c_str());
		canvas->update();
	};

private:
	QLabel *_label;
	const NDIlib_v4* _ndiLib;
	NDIPTZDeviceManager* _manager;
};
PTZControllerWidget *g_controllerdialog;
void ptz_controller_init(const NDIlib_v4* ndiLib, NDIPTZDeviceManager *manager)
{
    blog(LOG_INFO, "[patizo] obs_module_load: ptz_controller_init");

    g_controllerdialog = new PTZControllerWidget(ndiLib, manager);

    obs_frontend_add_dock_by_id(
	    obs_module_text("PatizoController"),
	    obs_module_text("Patizo Controller"), g_controllerdialog);

    blog(LOG_INFO, "[patizo] obs_module_load: PTZ Controller Dock added");
};
