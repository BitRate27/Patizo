#include "ptz-presets-dock.h"

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

PTZPresetsWidget *g_dialog;
void ptz_presets_init(const NDIlib_v4 *ndiLib, NDIPTZDeviceManager *manager)
{
	g_dialog = new PTZPresetsWidget(ndiLib, manager);

    obs_frontend_add_dock_by_id(
	   "PatizoPresets",
	    obs_module_text("PatizoPlugin.PresetsDock.Title"), g_dialog);

	blog(LOG_INFO, "[patizo] obs_module_load: Patizo Presets Dock added");
}
