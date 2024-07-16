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
#include "ndi-dock.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("ndi-dock", "en-US")

bool obs_module_load(void) {
    if (!NDIlib_initialize()) {
        blog(LOG_ERROR, "Failed to initialize NDI library");
        return false;
    }

    QMainWindow *main_window = (QMainWindow*)obs_frontend_get_main_window();
    NDIDock *dock = new NDIDock(main_window);
    obs_frontend_add_dock_by_id("PatizoDock",
		"patizo-dock",
		dock);
	blog(LOG_INFO, "[patizo] Patizo dock added");
    return true;
}

void obs_module_unload() {
    NDIlib_destroy();
}
