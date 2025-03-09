#include "ptz-presets-dock.h"
#include <obs-frontend-api.h>

PTZPresetsWidget *g_dialog = nullptr;
const char *patizoDockID = "PatizoPresets";

void ptz_presets_init(const NDIlib_v4 *ndiLib, NDIPTZDeviceManager *manager)
{
	// Get the main window of OBS
	QWidget *mainWindow = (QWidget *)obs_frontend_get_main_window();

	g_dialog = new PTZPresetsWidget(mainWindow, ndiLib, manager);

    obs_frontend_add_dock_by_id(
		patizoDockID,
	    obs_module_text("PatizoPlugin.PresetsDock.Title"), g_dialog);

	blog(LOG_INFO, "[patizo] obs_module_load: Patizo Presets Dock added");
}

void ptz_presets_hotkey_function(void* priv, obs_hotkey_id id, obs_hotkey_t* hotkey, bool pressed)
{
	(void)id;
	(void)hotkey;

	if (pressed) {
		PresetButton *button = static_cast<PresetButton*>(priv);
        if (button) button->recallPreset();
    }
}
void ptz_presets_destroy()
{
	obs_frontend_remove_dock(patizoDockID);
	if (g_dialog) {
		//g_dialog->deleteLater();
		g_dialog = nullptr;
	}
	
	blog(LOG_INFO, "[patizo] obs_module_unload: Patizo Presets Dock removed");
}
