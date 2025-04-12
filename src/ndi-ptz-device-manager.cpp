
#include <obs-frontend-api.h>
#include <regex>
#include <obs-module.h>
#include "ndi-ptz-device-manager.h"

static void on_scene_changed(enum obs_frontend_event event, void *param)
{
	(void)event;
	NDIPTZDeviceManager *manager =
		static_cast<NDIPTZDeviceManager *>(param);
	manager->onSceneChanged();
};

std::vector<obs_source_t *> getSourcesInScene(obs_source_t *scene_source)
{
	std::vector<obs_source_t *> sources;

	// Convert the source to a scene
	obs_scene_t *scene = obs_scene_from_source(scene_source);
	if (!scene) {
		return sources;
	}
	
	// Enumerate the items in the scene
	obs_scene_enum_items(
		scene,
		[](obs_scene_t *, obs_sceneitem_t *item, void *param) -> bool {
			auto *sources =
				static_cast<std::vector<obs_source_t *> *>(param);

			// Get the source from the scene item
			obs_source_t *source = obs_sceneitem_get_source(item);
			if (source) {
				std::string id = obs_source_get_id(source);
				if (id == "ndi_source") {
					obs_data_t *data =
						obs_source_get_settings(source);
					if (obs_data_get_bool(data,
							      "ndi_ptz")) {
						sources->push_back(source);
					}
				}
				if (id == "dshow_input") {
					obs_data_t *data =
						obs_source_get_settings(source);
					std::string video_device_id =
						obs_data_get_string(
							data,
							"video_device_id");
					sources->push_back(source);
				}
			}

			return true; // Continue enumeration
		},
		&sources);

	return sources;
}

void NDIPTZDeviceManager::init()
{
	obs_frontend_add_event_callback(on_scene_changed, this);
};

NDIPTZDeviceManager::~NDIPTZDeviceManager()
{
	closeAllConnections();
};

Receiver *NDIPTZDeviceManager::getRecvInfo(const std::string &ndi_name)
{
	auto it = _recvs.find(ndi_name);
	if (it != _recvs.end()) {
		return it->second;
	}
	// Return an empty Receiver
	return new Receiver();
};

std::string NDIPTZDeviceManager::getCurrent() const
{
	return _current;
};
size_t NDIPTZDeviceManager::registerRecvsChangedCallback(RecvsChangedCallback callback) {
    _recvsChangedCallbacks.push_back(callback);
    return _recvsChangedCallbacks.size() - 1;  // Return the index as an identifier
};
void NDIPTZDeviceManager::unregisterRecvsChangedCallback(size_t callbackId) {
    if (callbackId < _recvsChangedCallbacks.size()) {
        _recvsChangedCallbacks[callbackId] = nullptr;
    }
}

void NDIPTZDeviceManager::onSceneChanged()
{
    // Function to collect NDI names from a given scene
    auto collectNDISourcesFromScene = [this](obs_source_t* scene_source) {
        auto scene = obs_scene_from_source(scene_source);
        return createListOfNDISources(scene);
    };

    // Get all scenes
    obs_frontend_source_list source_list = {0};
    std::vector<obs_source_t*> all_ndi_sources;
	obs_frontend_get_scenes(&source_list);
    for (size_t i = 0; i < source_list.sources.num; ++i) {
        obs_source_t *source = source_list.sources.array[i];
        if (obs_source_get_type(source) == OBS_SOURCE_TYPE_SCENE) {
            std::vector<obs_source_t*> scene_ndi_sources = collectNDISourcesFromScene(source);
            all_ndi_sources.insert(all_ndi_sources.end(), scene_ndi_sources.begin(), scene_ndi_sources.end());
        }
    }

    obs_frontend_source_list_free(&source_list);

	updateRecvInfo(all_ndi_sources, _recvs);

	obs_source_t *preview_source = obs_frontend_get_current_preview_scene();

	auto preview_scene = obs_scene_from_source(preview_source);
	obs_source_release(preview_source);

	std::vector<obs_source_t*> preview_ndi_sources =
		createListOfNDISources(preview_scene);

	updateRecvInfo(preview_ndi_sources, _recvs);

	if ((preview_source != nullptr) && (preview_ndi_sources.size() == 0)) {
		_current = "";		
		_currentPreviewStatus = PreviewStatus::NotSupported;
		notifyCallbacks();
		return;
	}

	obs_source_t *program_source = obs_frontend_get_current_scene();
	auto program_scene = obs_scene_from_source(program_source);
	obs_source_release(program_source);

	std::vector<obs_source_t*> program_ndi_sources =
		createListOfNDISources(program_scene);

	if (preview_source != nullptr) {
		// Check if preview source also on program
		for (obs_source_t *source : preview_ndi_sources) {
			auto it = std::find(program_ndi_sources.begin(),
					    program_ndi_sources.end(), source);
			if (it != program_ndi_sources.end()) {
				_currentPreviewStatus = PreviewStatus::OnProgram;
				_current = "";
				notifyCallbacks();
				return;
			}
		}
	}

	// If there are preview ndi sources, then use the first one,
	// otherwise we are not in Studio mode, so allow preset recall on program
	// source.
	std::string ndi_name =
		(preview_ndi_sources.size() > 0)   ? getNDIName(preview_ndi_sources[0])
		: (program_ndi_sources.size() > 0) ? getNDIName(program_ndi_sources[0])
						: "";

	if (ndi_name != "") {
		_current = ndi_name;
		_currentPreviewStatus = PreviewStatus::OK;
	} else {
		_current = "";
		_currentPreviewStatus = PreviewStatus::NotSupported;
	}
	notifyCallbacks();
};

void NDIPTZDeviceManager::closeAllConnections()
{
	for (auto &recv : _recvs) {
		if (recv.second->recv == nullptr)
			g_ndiLib->recv_destroy(recv.second->recv);
		if (recv.second->visca_supported) {
			visca_error_t verr =
				recv.second->visca->disconnectCamera();
			if (verr != VOK) {
				blog(LOG_ERROR,
				     "[patizo] Failed to disconnect camera. Error code: %d",
				     verr);
			}
		}
	}
	_recvs.clear();
};

std::vector<std::string> NDIPTZDeviceManager::getNDINames()
{
    std::vector<std::string> name_list;
    for (auto &recv : _recvs) {
          name_list.push_back(recv.first); 
    }
    return name_list;
};
std::vector<obs_source_t*> NDIPTZDeviceManager::createListOfNDISources(obs_scene_t* scene)
{
    std::vector<obs_source_t*> name_list;
    obs_scene_enum_items(scene, [](obs_scene_t*, obs_sceneitem_t* item, void* param) -> bool {
        auto sources = static_cast<std::vector<obs_source_t*>*>(param);
		if (!obs_sceneitem_visible(item))
			return true;
        obs_source_t* source = obs_sceneitem_get_source(item);
        std::string ndi_name = getNDIName(source);
        if (ndi_name == "") return true;

        blog(LOG_INFO, "[patizo] NDI name: %s, showing: %d", ndi_name.c_str(), obs_source_showing(source));
        if (!obs_source_showing(source)) return true;
        sources->push_back(source);
        return true;
    }, &name_list);
    return name_list;
};

void NDIPTZDeviceManager::updateRecvInfo(const std::vector<obs_source_t *> source_list, 
                    std::map<std::string, Receiver*>& recvs)
{
    bool changed = false;
    for (obs_source_t* source : source_list) {
	    std::string ndi_name = getNDIName(source);
        auto it = recvs.find(ndi_name);
        if (it == recvs.end()) {
			Receiver *recv_info = new Receiver();
			recv_info->connect(source);
			recvs[recv_info->ndi_name] = recv_info;
			changed = true;
        }
    }
    if (changed) {
        for (const auto& callback : _recvsChangedCallbacks) {
            if (callback) {  // Check if the callback is valid
                callback();
            }
        }
    }
};
