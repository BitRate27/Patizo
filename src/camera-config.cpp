#include "camera-config.h"
#define PTZ_CAMERA_CONFIG "ptz_config"
#define PTZ_MIN_ZOOM "min_zoom"
#define PTZ_MAX_ZOOM "max_zoom"

CameraConfig *getSourceCameraConfig(const obs_source_t *source)
{
	obs_data_t *data = obs_source_get_settings(source);
	auto configObj = obs_data_get_obj(data, PTZ_CAMERA_CONFIG);
	auto config = new CameraConfig();
	if (!configObj) {
		setSourceCameraConfig(source, *config);
	} else {
		config->min_zoom = (int)obs_data_get_int(configObj, PTZ_MIN_ZOOM);
		config->max_zoom = (int)obs_data_get_int(configObj, PTZ_MAX_ZOOM);
		obs_data_release(configObj);
	}

	obs_data_release(data);
	return config;
}
void setSourceCameraConfig(const obs_source_t *source, CameraConfig &config)
{
	obs_data_t *data = obs_source_get_settings(source);
	auto configObj = obs_data_get_obj(data, PTZ_CAMERA_CONFIG);
	if (!configObj) {
		configObj = obs_data_create();
		obs_data_set_obj(data, PTZ_CAMERA_CONFIG, configObj);
	}
	obs_data_set_int(configObj, PTZ_MIN_ZOOM, config.min_zoom);
	obs_data_set_int(configObj, PTZ_MAX_ZOOM, config.max_zoom);
	obs_data_release(configObj);
	obs_data_release(data);
}