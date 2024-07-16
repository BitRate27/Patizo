#include "ndi-dock.h"
#include "qboxlayout.h"
#include "qmainwindow.h"
#include "qpixmap.h"

NDIReceiver::NDIReceiver() : pNDI_recv(nullptr), running(false) {}

NDIReceiver::~NDIReceiver() {
    stop();
}

void NDIReceiver::connectToSource(const QString& sourceName) {
    QMutexLocker locker(&mutex);
    
    if (pNDI_recv) {
        NDIlib_recv_destroy(pNDI_recv);
        pNDI_recv = nullptr;
    }

    NDIlib_source_t selected_source;
    selected_source.p_ndi_name = sourceName.toUtf8().constData();

    NDIlib_recv_create_v3_t recv_create_desc;
    recv_create_desc.source_to_connect_to = selected_source;
    recv_create_desc.color_format = NDIlib_recv_color_format_BGRA_RGBA;

    pNDI_recv = NDIlib_recv_create_v3(&recv_create_desc);

    if (pNDI_recv) {
        running = true;
        captureFrames();
    }
}

void NDIReceiver::stop() {
    QMutexLocker locker(&mutex);
    running = false;
    if (pNDI_recv) {
        NDIlib_recv_destroy(pNDI_recv);
        pNDI_recv = nullptr;
    }
}

void NDIReceiver::captureFrames() {
    while (running) {
        QMutexLocker locker(&mutex);
        if (!pNDI_recv) break;

        NDIlib_video_frame_v2_t video_frame;
        NDIlib_frame_type_e frame_type = NDIlib_recv_capture_v2(pNDI_recv, &video_frame, nullptr, nullptr, 0);

        if (frame_type == NDIlib_frame_type_video) {
            QImage image(video_frame.p_data, video_frame.xres, video_frame.yres, QImage::Format_RGBA8888);
            emit frameReady(image.copy()); // Create a deep copy of the image

            NDIlib_recv_free_video_v2(pNDI_recv, &video_frame);
        }

        locker.unlock();
        QThread::msleep(33); // ~30 fps
    }
}

NDIDock::NDIDock(QWidget *parent) : QWidget(parent) {
    setWindowTitle("NDI Source Dock");

    QWidget *container = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(container);

    sourceList = new QComboBox(container);
    refreshButton = new QPushButton("Refresh Sources", container);
    connectButton = new QPushButton("Connect", container);
    videoLabel = new QLabel(container);
    videoLabel->setMinimumSize(320, 180);

    layout->addWidget(sourceList);
    layout->addWidget(refreshButton);
    layout->addWidget(connectButton);
    layout->addWidget(videoLabel);

    setWidget(container);

    connect(refreshButton, &QPushButton::clicked, this, &NDIDock::refreshSources);
    connect(connectButton, &QPushButton::clicked, this, &NDIDock::connectToSource);

    ndiReceiver = new NDIReceiver();
    ndiReceiver->moveToThread(&receiverThread);

    connect(&receiverThread, &QThread::finished, ndiReceiver, &QObject::deleteLater);
    connect(this, &NDIDock::destroyed, &receiverThread, &QThread::quit);
    connect(ndiReceiver, &NDIReceiver::frameReady, this, &NDIDock::updateFrame);
    obs_frontend_add_dock_by_id("PatizoDock",
		"patizo-dock",
		this);
    receiverThread.start();
}

NDIDock::~NDIDock() {
    receiverThread.quit();
    receiverThread.wait();
}

void NDIDock::refreshSources() {
    sourceList->clear();

    NDIlib_find_instance_t pNDI_find = NDIlib_find_create_v2();
    if (!pNDI_find) return;

    uint32_t no_sources = 0;
    const NDIlib_source_t* p_sources = nullptr;
    
    while (!p_sources) {
        NDIlib_find_wait_for_sources(pNDI_find, 1000);
        p_sources = NDIlib_find_get_current_sources(pNDI_find, &no_sources);
    }

    for (uint32_t i = 0; i < no_sources; i++) {
        sourceList->addItem(p_sources[i].p_ndi_name);
    }

    NDIlib_find_destroy(pNDI_find);
}

void NDIDock::connectToSource() {
    QString selectedSource = sourceList->currentText();
    QMetaObject::invokeMethod(ndiReceiver, "connectToSource", Q_ARG(QString, selectedSource));
}

void NDIDock::updateFrame(QImage image) {
    videoLabel->setPixmap(QPixmap::fromImage(image).scaled(videoLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
