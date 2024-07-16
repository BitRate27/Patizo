#pragma once

#include "obs-module.h"
#include "obs-frontend-api.h"
#include "qcombobox.h"
#include "qpushbutton.h"
#include "qlabel.h"
#include "qthread.h"
#include "qmutex.h"
#include <Processing.NDI.Lib.h>

class NDIReceiver : public QObject {
    Q_OBJECT

public:
    NDIReceiver();
    ~NDIReceiver();

public slots:
    void connectToSource(const QString& sourceName);
    void stop();

signals:
    void frameReady(QImage image);

private:
    void captureFrames();

    NDIlib_recv_instance_t pNDI_recv;
    QMutex mutex;
    bool running;
};

class NDIDock : public QWidget {
    Q_OBJECT

public:
    NDIDock(QWidget *parent = nullptr);
    ~NDIDock();

private slots:
    void refreshSources();
    void connectToSource();
    void updateFrame(QImage image);

private:
    QComboBox *sourceList;
    QPushButton *refreshButton;
    QPushButton *connectButton;
    QLabel *videoLabel;
    
    QThread receiverThread;
    NDIReceiver *ndiReceiver;
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("ndi-dock", "en-US")