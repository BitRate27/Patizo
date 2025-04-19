#pragma once
#include <obs-frontend-api.h>
#include "../lib/visca27/ViscaAPI.h"
#include "ndi-ptz-device-manager.h"
#include "interactive-canvas.h"
#include <string>
#include <qwidget.h>
#include <qobject.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qthread.h>
#include <qpushbutton.h>
#include <qicon.h>
#include <qdialog.h>
#include <qformlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qgroupbox.h>
#include <qhboxlayout>
#include <qmessagebox.h>
#include <map>

// Structure to hold network settings
struct NetworkSettings {
	std::string ipAddress = "127.0.0.1";
	int port = 3456; // Default VISCA port
	bool useTCP = true;
};

extern NetworkSettings *getSourceNetworkSettings(const obs_source_t *source);
extern void setSourceNetworkSettings(const obs_source_t *source,
				     NetworkSettings &settings);

class SourceSettingsDialog : public QDialog {
    Q_OBJECT
public:
    SourceSettingsDialog(QWidget *parent, obs_source_t *source)
        : QDialog(parent), _source(source)
    {
        setWindowTitle("Source Network Settings");
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        QFormLayout *formLayout = new QFormLayout();

        if (_source) {
            _sourceName = obs_source_get_name(_source);
            QLabel *titleLabel = new QLabel(QString("Network Settings for: %1").arg(_sourceName.c_str()));
            titleLabel->setStyleSheet("font-weight: bold;");
            mainLayout->addWidget(titleLabel);
            
            // IP Address field
            _ipAddressEdit = new QLineEdit();
            formLayout->addRow("IP Address:", _ipAddressEdit);
            
            // Port field
            _portSpinBox = new QSpinBox();
            _portSpinBox->setMinimum(1);
            _portSpinBox->setMaximum(65535);
            formLayout->addRow("Port:", _portSpinBox);
            
            // Protocol selection
            QGroupBox *protocolGroup = new QGroupBox("Protocol");
            QHBoxLayout *protocolLayout = new QHBoxLayout();
            
            _tcpRadio = new QRadioButton("TCP");
            _udpRadio = new QRadioButton("UDP");
            
            _buttonGroup = new QButtonGroup(this);
            _buttonGroup->addButton(_tcpRadio);
            _buttonGroup->addButton(_udpRadio);
            
            protocolLayout->addWidget(_tcpRadio);
            protocolLayout->addWidget(_udpRadio);
            protocolGroup->setLayout(protocolLayout);
            
            formLayout->addRow(protocolGroup);
            
            // Load existing settings if available
            loadSettings();
        } else {
            formLayout->addRow(new QLabel("No source selected"));
        }
        
        mainLayout->addLayout(formLayout);
        
        // Buttons
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *saveButton = new QPushButton("Save");
        QPushButton *cancelButton = new QPushButton("Cancel");
        
        buttonLayout->addWidget(saveButton);
        buttonLayout->addWidget(cancelButton);
        
        mainLayout->addLayout(buttonLayout);
        
        connect(saveButton, &QPushButton::clicked, this, &SourceSettingsDialog::saveSettings);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        
        setLayout(mainLayout);
        resize(400, 250);
    }

private slots:
    void saveSettings() {
        if (!_source) return;
        
        NetworkSettings settings;
        settings.ipAddress = _ipAddressEdit->text().toStdString();
        settings.port = _portSpinBox->value();
        settings.useTCP = _tcpRadio->isChecked();
        
        // Validate IP address (basic validation)
        if (settings.ipAddress.empty()) {
            QMessageBox::warning(this, "Invalid Settings", "IP Address cannot be empty.");
            return;
        }
        
        // Save settings to the map
        setSourceNetworkSettings(_source,settings);
        
        blog(LOG_INFO, "Saved network settings for %s: IP=%s, Port=%d, Protocol=%s", 
             _sourceName.c_str(), settings.ipAddress.c_str(), settings.port, 
             settings.useTCP ? "TCP" : "UDP");
        
        accept();
    }

private:
    void loadSettings() {
        auto settings = getSourceNetworkSettings(_source);
       
        _ipAddressEdit->setText(QString::fromStdString(settings->ipAddress));
        _portSpinBox->setValue(settings->port);
        if (settings->useTCP)
            _tcpRadio->setChecked(true);
        else
            _udpRadio->setChecked(true);
    }

    obs_source_t *_source;

    std::string _sourceName;
    
    QLineEdit *_ipAddressEdit;
    QSpinBox *_portSpinBox;
    QRadioButton *_tcpRadio;
    QRadioButton *_udpRadio;
    QButtonGroup *_buttonGroup;
};

class PTZControllerWidget : public QWidget {
	Q_OBJECT
public:
	PTZControllerWidget(QWidget *parent, const NDIlib_v4 *ndiLib,
			    NDIPTZDeviceManager *manager)
		: QWidget(parent),
		  _ndiLib(ndiLib),
		  _manager(manager)
	{
		QVBoxLayout *vbox = new QVBoxLayout(this);
        
        // Create a horizontal layout for the combobox and settings button
        QHBoxLayout *sourceSelectionLayout = new QHBoxLayout();
        
		_sourceComboBox = new QComboBox();
        _sourceComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sourceSelectionLayout->addWidget(_sourceComboBox);
        
        // Create the settings button with a gear icon
        _settingsButton = new QPushButton();
        _settingsButton->setIcon(QIcon::fromTheme("configure", QIcon::fromTheme("preferences-system")));
        _settingsButton->setToolTip("Edit source network settings");
        _settingsButton->setFixedSize(24, 24);
        _settingsButton->setEnabled(false); // Initially disabled until a source is selected
        sourceSelectionLayout->addWidget(_settingsButton);
        
        vbox->addLayout(sourceSelectionLayout);

		connect(_sourceComboBox,
			QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &PTZControllerWidget::onSourceSelected);
        connect(_settingsButton, &QPushButton::clicked,
                this, &PTZControllerWidget::onSettingsButtonClicked);
            
		_canvas = new InteractiveCanvas(this,manager);
		_canvas->setSizePolicy(QSizePolicy::Expanding,
				       QSizePolicy::Expanding);
		vbox->addWidget(_canvas);
		setLayout(vbox);
		_canvas->show();
	};
	void updateSourceList()
	{
		obs_source_t *preview_source =
			obs_frontend_get_current_preview_scene();
		_sourceComboBox->clear();
		_sources.clear();
        _settingsButton->setEnabled(false);
        
		if (preview_source) {
			_sources = getSourcesInScene(preview_source);
			obs_source_release(preview_source);

			// Print the source names
			for (const obs_source_t *source : _sources) {
				std::string name = obs_source_get_name(source);
				_sourceComboBox->addItem(name.c_str());
				blog(LOG_INFO, "Source: %s", name.c_str());
			}
            
            // Enable the settings button if there are items in the combobox
            if (_sourceComboBox->count() > 0) {
                _settingsButton->setEnabled(true);
            }
		}
	};
    
  

protected:
private slots:
	void onSourceSelected(int index)
	{
		if (index >= 0 && index < (int)_sources.size()) {
			obs_source_t *selectedSource = _sources[index];
			std::string name = obs_source_get_name(selectedSource);
			blog(LOG_INFO, "Selected Source: %s", name.c_str());
			_canvas->setSource(selectedSource);
            _settingsButton->setEnabled(true);
		} else {
            _settingsButton->setEnabled(false);
        }
	}
    
    void onSettingsButtonClicked()
    {
        size_t index = _sourceComboBox->currentIndex();
        if (index >= 0 && (uint)index < _sources.size()) {
            obs_source_t *selectedSource = _sources[index];
            SourceSettingsDialog dialog(this, selectedSource);
            dialog.exec();
            
            // After settings are updated, we might want to refresh or update connections
            // This would depend on your implementation
            std::string sourceName = obs_source_get_name(selectedSource);
            auto settings = getSourceNetworkSettings(selectedSource);
            blog(LOG_INFO, "Using network settings for %s: IP=%s, Port=%d, Protocol=%s", 
                 sourceName.c_str(), settings->ipAddress.c_str(), settings->port, 
                 settings->useTCP ? "TCP" : "UDP");
            
            // Here you would potentially reconnect or update network connections
            // based on the new settings
        }
    }

private:
	const NDIlib_v4 *_ndiLib;
	NDIPTZDeviceManager *_manager;
	size_t _callbackid;
	QComboBox *_sourceComboBox;
    QPushButton *_settingsButton;
	std::vector<obs_source_t *> _sources;
	InteractiveCanvas *_canvas;
    
    // Map to store network settings for each source
    std::map<std::string, NetworkSettings> _sourceNetworkSettings;
};
void ptz_controller_init(const NDIlib_v4 *ndiLib, NDIPTZDeviceManager *manager);
void ptz_controller_destroy();
