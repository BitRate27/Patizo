#pragma once
#include <obs-frontend-api.h>
// Structure to hold network settings
struct CameraConfig {
	int min_zoom = 100;
	int max_zoom = 400;
};
extern CameraConfig *getSourceCameraConfig(const obs_source_t *source);
extern void setSourceCameraConfig(const obs_source_t *source,
				     CameraConfig &config);