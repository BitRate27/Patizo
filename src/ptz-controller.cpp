#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include "ptz-controller.h"
#include "qlayout.h"
#include "qcombobox.h"

void controller_on_scene_changed(enum obs_frontend_event event,
					void *param)
{
	(void)event;
	PTZControllerWidget *dialog = static_cast<PTZControllerWidget *>(param);
	dialog->updateSourceList();
};

PTZControllerWidget *g_dialog = nullptr;
const char *patizoControllerDockID = "PatizoController";
std::map<std::string, NetworkSettings> _settingsMap;
// Get network settings for a source
NetworkSettings getSourceNetworkSettings(const std::string &sourceName)
{
	auto it = _settingsMap.find(sourceName);
	if (it != _settingsMap.end()) {
		return it->second;
	}
	// Return default settings if not found
	NetworkSettings defaultSettings;
	return defaultSettings;
}

void ptz_controller_init(const NDIlib_v4 *ndiLib, NDIPTZDeviceManager *manager)
{
	blog(LOG_INFO, "[patizo] obs_module_load: ptz_controller_init");
	// Get the main window of OBS
	QWidget *mainWindow = (QWidget *)obs_frontend_get_main_window();
	//auto hwnd = (HWND)mainWindow->winId();

	g_dialog = new PTZControllerWidget(mainWindow, ndiLib, manager);
	g_dialog->show();
	_settingsMap.clear();

	obs_frontend_add_dock_by_id(
		patizoControllerDockID,
		obs_module_text("PatizoPlugin.ControllerDock.Title"), g_dialog);

	obs_frontend_add_event_callback(controller_on_scene_changed, g_dialog);

	blog(LOG_INFO,
	     "[patizo] obs_module_load: Patizo Controller Dock added");
}

void ptz_controller_destroy()
{
	obs_frontend_remove_event_callback(controller_on_scene_changed, g_dialog);
	obs_frontend_remove_dock(patizoControllerDockID);
	if (g_dialog) {
		//g_dialog->deleteLater();
		g_dialog = nullptr;
	}

	blog(LOG_INFO,
	     "[patizo] obs_module_unload: Patizo controller removed");
}