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
void controller_on_scene_changed(enum obs_frontend_event event, void *param);

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
		_sourceComboBox = new QComboBox();
		vbox->addWidget(_sourceComboBox);

		obs_frontend_add_event_callback(controller_on_scene_changed,
						this);
		connect(_sourceComboBox,
			QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &PTZControllerWidget::onSourceSelected);
		_canvas = new InteractiveCanvas(this);
		_canvas->setSizePolicy(QSizePolicy::Expanding,
				       QSizePolicy::Expanding);
		vbox->addWidget(_canvas);
		setLayout(vbox);
		_canvas->show();
	};
	~PTZControllerWidget()
	{
		obs_frontend_remove_event_callback(controller_on_scene_changed,
						   this);
	};
	void updateSourceList()
	{
		obs_source_t *preview_source =
			obs_frontend_get_current_preview_scene();
		_sourceComboBox->clear();
		_sources.clear();
		if (preview_source) {
			_sources = getSourcesInScene(preview_source);
			obs_source_release(preview_source);

			// Print the source names
			for (const obs_source_t *source : _sources) {
				std::string name = obs_source_get_name(source);
				_sourceComboBox->addItem(name.c_str());
				blog(LOG_INFO, "Source: %s", name.c_str());
			}
		}
	};

protected:
private slots:
	void onSourceSelected(int index)
	{
		if (index >= 0 && index < _sources.size()) {
			obs_source_t *selectedSource = _sources[index];
			std::string name = obs_source_get_name(selectedSource);
			blog(LOG_INFO, "Selected Source: %s", name.c_str());
			_canvas->setSource(selectedSource);
		}
	}

private:
	const NDIlib_v4 *_ndiLib;
	NDIPTZDeviceManager *_manager;
	size_t _callbackid;
	QComboBox *_sourceComboBox;
	std::vector<obs_source_t *> _sources;
	InteractiveCanvas *_canvas;
};
void ptz_controller_init(const NDIlib_v4 *ndiLib, NDIPTZDeviceManager *manager);
