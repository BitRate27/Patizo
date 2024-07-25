#pragma once
#include <Processing.NDI.Lib.h>
#include "ViscaAPI.h"
#include <QMetaObject>
#include <QThread>
#include <QObject>
#include <pthread.h>
#include <string>
#include <qwidget.h>
#include <mutex>
#include "ndi-ptz-device-manager.h"

class InteractiveCanvas : public QWidget {
	Q_OBJECT
public:
	explicit InteractiveCanvas(QWidget *parent = nullptr,
				   const NDIlib_v4 *ndiLib = nullptr,
				   NDIPTZDeviceManager *manager = nullptr);
public slots:
	void updateImage(int dummy);
protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
private:
	QImage canvas;
	void resizeEvent(QResizeEvent *event) override;
	void resizeCanvas(const QSize &newSize);
	QRect imageRect(QImage &image) const;
	const NDIlib_v4 *_ndiLib;
	NDIPTZDeviceManager *_manager;
};

void ptz_controller_init(
	const NDIlib_v4 *ndiLib,
	NDIPTZDeviceManager *manager);
