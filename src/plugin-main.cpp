/*
Plugin Name
Copyright (C) <Year> <Developer> <Email Address>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>
#include <QMainWindow>
#include <QLibrary>
#include <QDir>
#include <QFileInfo>
#include "plugin-main.h"
#include "ndi-ptz-device-manager.h"
#include "ptz-presets-dock.h"
#include "ptz-controller.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("patizo", "en-US")

const NDIlib_v5 *load_ndilib();
NDIPTZDeviceManager *g_ndiptz;
const NDIlib_v4 *g_ndiLib;
typedef const NDIlib_v5 *(*NDIlib_v5_load_)(void);
QLibrary *loaded_lib = nullptr;

NDIlib_find_instance_t ndi_finder = nullptr;
bool obs_module_load(void) {
	
    g_ndiLib = load_ndilib();
	if (!g_ndiLib) {
		blog(LOG_ERROR,
		     "[patizo] obs_module_load: load_ndilib() failed; Module won't load.");
	}

    if (!g_ndiLib->initialize()) {
        blog(LOG_ERROR, "Failed to initialize NDI library");
        return false;
    }

	g_ndiptz = new NDIPTZDeviceManager();
	g_ndiptz->init(g_ndiLib);

	ptz_presets_init(g_ndiLib, g_ndiptz);
	blog(LOG_INFO, "[patizo] Patizo presets dock added");
	//ptz_controller_init(g_ndiLib, g_ndiptz);
	//blog(LOG_INFO, "[patizo] Patizo controller dock added");
    return true;
}

void obs_module_unload() {
    g_ndiLib->destroy();
}

const NDIlib_v4 *load_ndilib()
{
	QStringList locations;
	QString path = QString(qgetenv(NDILIB_REDIST_FOLDER));
	if (!path.isEmpty()) {
		locations << path;
	}
#if defined(__linux__) || defined(__APPLE__)
	locations << "/usr/lib";
	locations << "/usr/local/lib";
#endif
	for (QString location : locations) {
		path = QDir::cleanPath(
			QDir(location).absoluteFilePath(NDILIB_LIBRARY_NAME));
		blog(LOG_INFO, "[patizo] load_ndilib: Trying '%s'",
		     path.toUtf8().constData());
		QFileInfo libPath(path);
		if (libPath.exists() && libPath.isFile()) {
			path = libPath.absoluteFilePath();
			blog(LOG_INFO,
			     "[patizo] load_ndilib: Found NDI library at '%s'",
			     path.toUtf8().constData());
			loaded_lib = new QLibrary(path, nullptr);
			if (loaded_lib->load()) {
				blog(LOG_INFO,
				     "[patizo] load_ndilib: NDI runtime loaded successfully");
				NDIlib_v5_load_ lib_load =
					(NDIlib_v5_load_)loaded_lib->resolve(
						"NDIlib_v5_load");
				if (lib_load != nullptr) {
					blog(LOG_INFO,
					     "[patizo] load_ndilib: NDIlib_v5_load found");
					return lib_load();
				} else {
					blog(LOG_ERROR,
					     "[patizo] load_ndilib: ERROR: NDIlib_v5_load not found in loaded library");
				}
			} else {
				blog(LOG_ERROR,
				     "[patizo] load_ndilib: ERROR: QLibrary returned the following error: '%s'",
				     loaded_lib->errorString()
					     .toUtf8()
					     .constData());
				delete loaded_lib;
				loaded_lib = nullptr;
			}
		}
	}
	blog(LOG_ERROR,
	     "[patizo] load_ndilib: ERROR: Can't find the NDI library");
	return nullptr;
};
