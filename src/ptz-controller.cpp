#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include "ptz-controller.h"
#include "qlayout.h"
#include "qcombobox.h"

static void controller_on_scene_changed(enum obs_frontend_event event,
					void *param)
{
	(void)event;
	PTZControllerWidget *dialog = static_cast<PTZControllerWidget *>(param);
	dialog->updateSourceList();
};

PTZControllerWidget *g_dialog = nullptr;
const char *patizoControllerDockID = "PatizoController";

void ptz_controller_init(const NDIlib_v4 *ndiLib, NDIPTZDeviceManager *manager)
{
	blog(LOG_INFO, "[patizo] obs_module_load: ptz_controller_init");
	// Get the main window of OBS
	QWidget *mainWindow = (QWidget *)obs_frontend_get_main_window();
	auto hwnd = (HWND)mainWindow->winId();

	g_dialog = new PTZControllerWidget(mainWindow, ndiLib, manager);
	g_dialog->show();

	obs_frontend_add_dock_by_id(
		patizoControllerDockID,
		obs_module_text("PatizoPlugin.ControllerDock.Title"), g_dialog);

	blog(LOG_INFO,
	     "[patizo] obs_module_load: Patizo Controller Dock added");
}