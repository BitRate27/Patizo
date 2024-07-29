
#include <chrono>
#include <obs.h>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <pthread.h>
#include <qgridlayout.h>
#include <qlabel.h>
#include <QMouseEvent>
#include <qpainter.h>
#include <qpushbutton.h>
#include <thread>
#include <qlineedit.h>
#include "ptz-presets-dock.h"

#define PROP_PRESET "preset%1"
#define PROP_NPRESETS 9

#define MAX_PRESET_NAME_LENGTH 12

class PresetButton : public QPushButton {
public:
	inline PresetButton(QWidget *parent_, int index_, const NDIlib_v4* ndiLib, NDIPTZDeviceManager* manager)
		: QPushButton(parent_),
		index(index_), _ndiLib(ndiLib), _manager(manager)
	{
		this->setText("");
		this->setSizePolicy(QSizePolicy::Expanding,
							QSizePolicy::Expanding);
		QObject::connect(this, &QPushButton::clicked, this,
						&PresetButton::PresetButtonClicked);
	}
	inline void PresetButtonClicked() { 
		_ndiLib->recv_ptz_recall_preset(_manager->getRecvInfo(_manager->getCurrent()).recv,
						  index,
						  5); 
	}
	int index;
	const NDIlib_v4* _ndiLib;
	NDIPTZDeviceManager* _manager;
};
class PTZPresetsWidget : public QWidget {
public:
	PTZPresetsWidget(const NDIlib_v4* ndiLib, NDIPTZDeviceManager* manager) : 
		_ndiLib(ndiLib), _manager(manager)
	{
		_label = new QLabel("");
		_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		QVBoxLayout *vbox = new QVBoxLayout(this);
		vbox->addWidget(_label);

		QGridLayout *grid = new QGridLayout();
		vbox->addLayout(grid);

		_nrows = 3;
		_ncols = 3;
		_buttons = (PresetButton **)bzalloc(
			sizeof(PresetButton *) * _nrows * _ncols);

		for (int i = 0; i < _nrows; ++i) {
			for (int j = 0; j < _ncols; ++j) {
				int ndx = i * _ncols + j;
				_buttons[ndx] =
					new PresetButton(this, ndx + 1, _ndiLib, _manager);
				_buttons[ndx]->setEnabled(true);
				grid->addWidget(_buttons[ndx], i, j);
				/*
				obs_hotkey_id hotkeyId = 
					obs_hotkey_register_frontend(QString("PTZPreset%1").arg(ndx+1).toUtf8(),
												QString("PTZ Preset %1").arg(ndx+1).toUtf8(), 
												ptz_presets_hotkey_function, 
												(void*)_buttons[ndx]);
												*/
			}
		}
		this->setLayout(grid);

		_button_pressed = -1;
	}
protected:
	void paintEvent(QPaintEvent *event) override
	{		
		if (!event) return;

		QPainter painter(this);

		_label->setText(_manager->getCurrent().c_str());

		for (int b = 0; b < PROP_NPRESETS; ++b) {
			if (_manager->getCurrent() != "") {
				_buttons[b]->setEnabled(true);
				_buttons[b]->setText(QString("Preset %1").arg(b+1).toUtf8());
				/*
					obs_data_get_string(
						settings,
						QString(PROP_PRESET)
							.arg(b+1)
							.toUtf8()));
				*/
			} else {
				_buttons[b]->setEnabled(false);
				_buttons[b]->setText("");
			}
		}

		//obs_data_release(settings);
	};
private:
	QLabel *_label;
	const NDIlib_v4* _ndiLib;
	NDIPTZDeviceManager* _manager;
	PresetButton **_buttons;
	int _ncols;
	int _nrows;
	int _button_pressed;
};

bool ptz_presets_property_modified(void *priv, obs_properties_t *props,
		       obs_property_t *property, obs_data_t *settings )
{
	//bool changed = false;
	(void)props;
	PresetButton *button = static_cast<PresetButton*>(priv);
    const char* property_name = obs_property_name(property);
    const char* value = obs_data_get_string(settings, property_name);
    if (strlen(value) > MAX_PRESET_NAME_LENGTH) {
	    std::string temp = value;
	    obs_data_set_string(settings, obs_property_name(property),
				temp.substr(0, MAX_PRESET_NAME_LENGTH).c_str());
	    button->setText(temp.c_str());
	    return true;
    }

	button->setText(value);

    return false;
}

void ptz_presets_hotkey_function(void* priv, obs_hotkey_id id, obs_hotkey_t* hotkey, bool pressed)
{
	(void)id;
	(void)hotkey;
	(void)pressed;

	if (pressed) {
		PresetButton *button = static_cast<PresetButton*>(priv);
		(void)button;
        //ptz_preset_button_pressed(button->index);
    }
}
/**
void ptz_presets_add_properties(obs_properties_t *group_ptz){
	for (int pp = 1; pp <= PROP_NPRESETS; pp++) {
		auto p = obs_properties_add_text(
			group_ptz, QString(PROP_PRESET).arg(pp).toUtf8(),
			QString("Preset %1").arg(pp).toUtf8(),
			OBS_TEXT_DEFAULT);

		obs_property_set_modified_callback2(
			p, 
			(obs_property_modified2_t)ptz_presets_property_modified, 
			(void*)_buttons[pp-1]);
	}
};
*/

void ptz_presets_set_defaults(obs_data_t *settings)
{
	for (int pp = 1; pp <= PROP_NPRESETS; pp++) {
		obs_data_set_default_string(
			settings, QString(PROP_PRESET).arg(pp).toUtf8(),
			QString("Preset %1").arg(pp).toUtf8());
	}
}

PTZPresetsWidget *g_dialog;
void ptz_presets_init(const NDIlib_v4 *ndiLib, NDIPTZDeviceManager *manager)
{
	g_dialog = new PTZPresetsWidget(ndiLib, manager);

    obs_frontend_add_dock_by_id(
	    obs_module_text("PTZController"),
	    obs_module_text("PTZ Controller"), g_dialog);

	blog(LOG_INFO, "[obs-ndi] obs_module_load: PTZ Presets Dock added");
}
