#include "ptz-presets-dock.h"

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


void ptz_presets_set_defaults(obs_data_t *settings)
{
	for (int pp = 1; pp <= PROP_NPRESETS; pp++) {
		obs_data_set_default_string(
			settings, QString(PROP_PRESET).arg(pp).toUtf8(),
			QString(obs_module_text("PatizoPlugin.PresetsDock.Default")).arg(pp).toUtf8());
	}
}
*/
PTZPresetsWidget *g_dialog;
void ptz_presets_init(const NDIlib_v4 *ndiLib, NDIPTZDeviceManager *manager)
{
	g_dialog = new PTZPresetsWidget(ndiLib, manager);

    obs_frontend_add_dock_by_id(
	   "PatizoPresets",
	    obs_module_text("PatizoPlugin.PresetsDock.Title"), g_dialog);

	blog(LOG_INFO, "[obs-ndi] obs_module_load: Patizo Presets Dock added");
}
