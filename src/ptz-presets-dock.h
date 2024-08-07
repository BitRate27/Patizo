#pragma once
#include <obs-module.h>
#include <obs.h>
#include <obs-properties.h>
#include <chrono>
#include <obs-frontend-api.h>
#include "util/platform.h"
#include <pthread.h>
#include <qgridlayout.h>
#include <qlabel.h>
#include <QMouseEvent>
#include <qcombobox.h>
#include <qpainter.h>
#include <qpushbutton.h>
#include <thread>
#include <qlineedit.h>
#include <obs-source.h>
#include <string>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QTimer>
#include "ndi-ptz-device-manager.h"
#include <Processing.NDI.Lib.h>
#include <qelapsedtimer.h>

#define PROP_PRESET "preset%1"
#define PROP_NPRESETS 9

void ptz_presets_hotkey_function(void* priv, obs_hotkey_id id, obs_hotkey_t* hotkey, bool pressed);

class PresetButton : public QWidget {
    Q_OBJECT

public:
    PresetButton(QWidget *parent_, int index_, const NDIlib_v4* ndiLib, NDIPTZDeviceManager* manager)
        : QWidget(parent_), index(index_), _ndiLib(ndiLib), _manager(manager)
    {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        _label = new QLabel(this);
        _label->setWordWrap(true);
        _label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        _label->setAlignment(Qt::AlignCenter);
        _label->setAttribute(Qt::WA_TranslucentBackground, true); // Set background to be transparent


        _lineEdit = new QLineEdit(this);
        _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        _lineEdit->setAlignment(Qt::AlignCenter);
        _lineEdit->hide();
        
        layout->addWidget(_label);
        layout->addWidget(_lineEdit);
        setLayout(layout);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setAutoFillBackground(true);
        show();
        // connect(this, &QLabel::clicked, this, &PresetButton::handleClick);
        connect(_lineEdit, &QLineEdit::editingFinished, this, &PresetButton::finishEditing);
    }

    void setText(const QString &text) {
        _label->setText(text);
    }

    QString text() const {
        return _label->text();
    }

    void recallPreset() {
        auto recv = _manager->getRecvInfo(_manager->getCurrent()).recv;
        _ndiLib->recv_ptz_recall_preset(recv, index, 5);
    }

signals:
    void doubleClicked();
    void nameChanged(const QString &newName);

protected:

    void focusOutEvent(QFocusEvent *event) override {
        if (_lineEdit->isVisible()) {
            finishEditing();
        }
        QWidget::focusOutEvent(event);
    }

    void paintEvent(QPaintEvent *event) override
    {        
        if (!event) return;

        QPainter painter(this);
        painter.setPen(_backgroundColor);
        painter.setBrush(_backgroundColor); 
        painter.drawRect(2, 2, width()-4, height()-4); // Draw and fill the rectangle
    }
    
    bool event(QEvent *event) override {
        if (event->type() == QEvent::Enter) {
            _backgroundColor = palette().color(QPalette::Highlight);
            update();
        } else if (event->type() == QEvent::Leave) {
            _backgroundColor = palette().color(QPalette::Button);
            update(); 
        } else if (event->type() == QEvent::MouseButtonPress) {
            handleSingleClick();
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            startEditing(); 
        }
        
        return QWidget::event(event);
    }

private:
    QLabel *_label;
    QLineEdit *_lineEdit;
    int index;
    const NDIlib_v4* _ndiLib;
    NDIPTZDeviceManager* _manager;
    QColor _backgroundColor = palette().color(QPalette::Button);

    void handleSingleClick() {
        auto recv = _manager->getRecvInfo(_manager->getCurrent()).recv;
        _ndiLib->recv_ptz_recall_preset(recv, index, 5);
    }

    void startEditing() {
        _lineEdit->setText(_label->text());
        _label->hide();
        _lineEdit->show();
        _lineEdit->setFocus();
        _lineEdit->selectAll();
    }

    void finishEditing() {
        if (!_lineEdit->isVisible()) return;

        QString newName = _lineEdit->text();
        _label->setText(newName);
        _lineEdit->hide();
        _label->show();
        emit nameChanged(newName);
    }
};

class PTZPresetsWidget : public QWidget {
    Q_OBJECT

public:
    PTZPresetsWidget(const NDIlib_v4* ndiLib, NDIPTZDeviceManager* manager) : 
        _ndiLib(ndiLib), _manager(manager)
    {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
	    _label = new QLabel("");
	    _label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        _label->setMinimumHeight(_label->fontMetrics().height());  // Set fixed height to one line height
        _label->setMaximumHeight(_label->fontMetrics().height());
	    mainLayout->addWidget(_label);

        QGridLayout *grid = new QGridLayout();
        _gridWidget = new QWidget(this);
        _gridWidget->setContentsMargins(0, 0, 0, 0); // left, top, right, bottom

        // Set border
        _gridWidget->setStyleSheet("border: 0px solid black;");    
        _gridWidget->setLayout(grid);
        _gridWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        mainLayout->addWidget(_gridWidget);

        _nrows = 3;
        _ncols = 3;
        _buttons = new PresetButton*[_nrows * _ncols];

        for (int i = 0; i < _nrows; ++i) {
            for (int j = 0; j < _ncols; ++j) {
                int ndx = i * _ncols + j;
                _buttons[ndx] = new PresetButton(this, ndx + 1, _ndiLib, _manager);

                grid->addWidget(_buttons[ndx], i, j);
                obs_hotkey_id hotkeyId = 
					obs_hotkey_register_frontend(QString("PatizoPreset%1").arg(ndx+1).toUtf8(),
												QString("Patizo Preset %1 Recall").arg(ndx+1).toUtf8(), 
												ptz_presets_hotkey_function, 
												(void*)_buttons[ndx]);
                connect(_buttons[ndx], &PresetButton::nameChanged, this, [this, ndx](const QString &newName) {
                    handleNameChanged(ndx, newName);
                });
            }
        }

        // Set stretch factors for rows and columns to ensure equal expansion
        for (int i = 0; i < _nrows; ++i) {
            grid->setRowStretch(i, 1);
        }
        for (int j = 0; j < _ncols; ++j) {
            grid->setColumnStretch(j, 1);
        }

        // Allow the widget to expand
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setLayout(mainLayout);

        // Register the callback with the manager
        _callbackid = _manager->registerRecvsChangedCallback([this]() {

            if (_manager->getCurrentPreviewStatus() == 
                NDIPTZDeviceManager::PreviewStatus::OK) {
                _label->setText(_manager->getCurrent().c_str());
		        loadButtonNames(_manager->getCurrent().c_str());
            }
            else if (_manager->getCurrentPreviewStatus() == 
                NDIPTZDeviceManager::PreviewStatus::OnProgram) {
                _label->setText(obs_module_text("PatizoPlugin.Devices.OnProgram"));
            } else {
                _label->setText(obs_module_text("PatizoPlugin.Devices.NotSupported"));
            }
        });
    }

    ~PTZPresetsWidget() {
        delete[] _buttons;
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {        
        if (!event) return;

        QPainter painter(this);

        bool enabled = (_manager->getCurrentPreviewStatus() == 
            NDIPTZDeviceManager::PreviewStatus::OK);

        for (int b = 0; b < PROP_NPRESETS; ++b) {
		    if (enabled) {
                _buttons[b]->setEnabled(true);
                if (_buttonNames.contains(QString::number(b+1))) {
                    _buttons[b]->setText(_buttonNames[QString::number(b+1)].toString());
                } else {
                    _buttons[b]->setText(QString("Preset %1").arg(b+1).toUtf8());
                }
            } else {
                _buttons[b]->setEnabled(false);
                _buttons[b]->setText("");
            }
        }
    }

	void showEvent(QShowEvent *event) override {
        QWidget::showEvent(event);
        //populateComboBox();
    }

private:
    QLabel *_label;
    QComboBox *_comboBox;
	QCheckBox *_toggleButton;
    const NDIlib_v4* _ndiLib;
    NDIPTZDeviceManager* _manager;
    PresetButton **_buttons;
    QWidget *_gridWidget;
    int _ncols;
    int _nrows;
    QJsonObject _buttonNames;
	size_t _callbackid;

    void handleNameChanged(int buttonIndex, const QString &newName) {
        _buttonNames[QString::number(buttonIndex + 1)] = newName;

        if (_manager->getCurrentPreviewStatus() == 
            NDIPTZDeviceManager::PreviewStatus::OK) {
            saveButtonNames(QString::fromStdString(_manager->getCurrent()));
        }
    }

    QString getConfigFilePath()
    {
	    char *config_path = os_get_config_path_ptr("obs-studio");
	    if (!config_path) {
		    return QString();
	    }

	    QString path = QString::fromUtf8(config_path) +
			   "/plugin_config/patizo/";
	    bfree(config_path);
	    return path;
    }

    void loadButtonNames(const QString &ndiName)
    {
        QString filePath = getConfigFilePath() + "settings.json";
        _buttonNames = QJsonObject();
      
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray saveData = file.readAll();
            QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
            QJsonObject allButtonNames = loadDoc.object();
            if (allButtonNames.contains(ndiName)) {
                _buttonNames = allButtonNames.value(ndiName).toObject();
            }
            file.close();
        }
    }

    void saveButtonNames(const QString &ndiName)
    {
        QString filePath = getConfigFilePath() + "settings.json";
        if (filePath.isEmpty()) {
            blog(LOG_WARNING, "Failed to get config path for saving PTZ preset names");
            return;
        }

        QDir().mkpath(QFileInfo(filePath).path());
        QFile file(filePath);
        QJsonObject allButtonNames;
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray saveData = file.readAll();
            QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
            allButtonNames = loadDoc.object();
            file.close();
        }

        allButtonNames[ndiName] = _buttonNames;

        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument saveDoc(allButtonNames);
            file.write(saveDoc.toJson());
            file.close();
        }
    }
};
void ptz_presets_init(const NDIlib_v4 *, NDIPTZDeviceManager *manager);
