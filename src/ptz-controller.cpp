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

#define PTZ_SETTINGS "ptz_settings"
#define PTZ_IP_ADDRESS "ip_address"
#define PTZ_PORT "port"
#define PTZ_USE_TCP "use_tcp"
NetworkSettings *getSourceNetworkSettings(const obs_source_t *source)
{
	obs_data_t *data = obs_source_get_settings(source);
	auto settingsObj = obs_data_get_obj(data, PTZ_SETTINGS);		
	auto settings = new NetworkSettings();
	if (!settingsObj) {
		setSourceNetworkSettings(source, *settings);
	} else {
		settings->ipAddress =
			obs_data_get_string(settingsObj, PTZ_IP_ADDRESS);
		settings->port = obs_data_get_int(settingsObj, PTZ_PORT);
		settings->useTCP = obs_data_get_bool(settingsObj, PTZ_USE_TCP);	
		obs_data_release(settingsObj);
	}

	obs_data_release(data);
	return settings;
}
void setSourceNetworkSettings(const obs_source_t *source, NetworkSettings &settings)
{
	obs_data_t *data = obs_source_get_settings(source);
	auto settingsObj = obs_data_get_obj(data, PTZ_SETTINGS);
	if (!settingsObj) {
		settingsObj = obs_data_create();
		obs_data_set_obj(data, PTZ_SETTINGS, settingsObj);
	}
	obs_data_set_string(settingsObj, PTZ_IP_ADDRESS,
			    settings.ipAddress.c_str());
	obs_data_set_int(settingsObj, PTZ_PORT, settings.port);
	obs_data_set_bool(settingsObj, PTZ_USE_TCP, settings.useTCP);
	obs_data_release(settingsObj);
	obs_data_release(data);
}
void ptz_controller_init(const NDIlib_v4 *ndiLib, NDIPTZDeviceManager *manager)
{
	blog(LOG_INFO, "[patizo] obs_module_load: ptz_controller_init");
	// Get the main window of OBS
	QWidget *mainWindow = (QWidget *)obs_frontend_get_main_window();
	//auto hwnd = (HWND)mainWindow->winId();

	g_dialog = new PTZControllerWidget(mainWindow, ndiLib, manager);
	g_dialog->show();

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
