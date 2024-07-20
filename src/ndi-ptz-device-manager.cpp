
#include <obs-frontend-api.h>
#include <regex>
#include <obs-module.h>
#include "ndi-ptz-device-manager.h"

static void on_scene_changed(enum obs_frontend_event event, void *param)
{
	NDIPTZDeviceManager *manager =
		static_cast<NDIPTZDeviceManager *>(param);
	manager->onSceneChanged();
};

void NDIPTZDeviceManager::init(const NDIlib_v4 *ndiLib)
{
	_ndiLib = ndiLib;
	obs_frontend_add_event_callback(on_scene_changed, this);
};

NDIPTZDeviceManager::~NDIPTZDeviceManager()
{
	closeAllConnections();
};

recv_info_t NDIPTZDeviceManager::getRecvInfo(const std::string &ndi_name)
{
	auto it = _recvs.find(ndi_name);
	if (it != _recvs.end()) {
		return it->second;
	}
	// Return an empty struct or handle error
	return recv_info_t{};
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
	obs_source_t *preview_source = obs_frontend_get_current_preview_scene();

	auto preview_scene = obs_scene_from_source(preview_source);
	obs_source_release(preview_source);

	std::vector<std::string> preview_ndinames =
		createListOfNDINames(preview_scene);

	updateRecvInfo(_ndiLib, preview_ndinames, _recvs);

	if ((preview_source != nullptr) && (preview_ndinames.size() == 0)) {
		_current = obs_module_text(
			"NDIPlugin.PTZPresetsDock.NotSupported");
		return;
	}

	obs_source_t *program_source = obs_frontend_get_current_scene();
	auto program_scene = obs_scene_from_source(program_source);
	obs_source_release(program_source);

	std::vector<std::string> program_ndinames =
		createListOfNDINames(program_scene);

	updateRecvInfo(_ndiLib, program_ndinames, _recvs);

	if (preview_source != nullptr) {
		// Check if preview source also on program
		for (const std::string &name : preview_ndinames) {
			auto it = std::find(program_ndinames.begin(),
					    program_ndinames.end(), name);
			if (it != program_ndinames.end()) {
				_current = obs_module_text(
					"NDIPlugin.PTZPresetsDock.OnProgram");
				return;
			}
		}
	}

	// If there are preview ndi sources, then use the first one,
	// otherwise we are not in Studio mode, so allow preset recall on program
	// source.
	std::string ndi_name =
		(preview_ndinames.size() > 0)   ? preview_ndinames[0]
		: (program_ndinames.size() > 0) ? program_ndinames[0]
						: "";

	if (ndi_name != "") {
		_current = ndi_name;
	} else {
		_current = obs_module_text(
			"NDIPlugin.PTZPresetsDock.NotSupported");
	}
};

void NDIPTZDeviceManager::closeAllConnections()
{
	for (auto &recv : _recvs) {
		// Close Visca connection
		recv.second.visca.disconnectCamera();
	}
	_recvs.clear();
};
static std::string getNDIName(const obs_source_t *source)
{
	std::string id = obs_source_get_id(source);
	if (id != "ndi_source")
		return "";
	obs_data_t *data = obs_source_get_settings(source);
	return obs_data_get_string(data, "ndi_source_name");
};

static NDIlib_recv_instance_t getRecv(const NDIlib_v4 *ndiLib,
				      const std::string &ndi_name)
{
	NDIlib_recv_instance_t recv = nullptr;
	NDIlib_source_t selected_source;
	selected_source.p_ndi_name = ndi_name.data();

	NDIlib_recv_create_v3_t recv_create_desc;
	recv_create_desc.source_to_connect_to = selected_source;
	recv_create_desc.color_format = NDIlib_recv_color_format_RGBX_RGBA;

	recv = ndiLib->recv_create_v3(&recv_create_desc);
	return recv;
};
static std::string extractIPAddress(const std::string &str)
{
	std::regex ip_regex(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}))");
	std::smatch match;
	if (std::regex_search(str, match, ip_regex)) {
		return match[0];
	}
	return "";
};
static ViscaAPI getViscaAPI(const NDIlib_v4 *ndiLib,
			    NDIlib_recv_instance_t recv)
{
	if (!ndiLib->recv_ptz_is_supported(recv))
		return ViscaAPI();
	ViscaAPI visca;
	char ip[100];
	const char *p_url = ndiLib->recv_get_web_control(recv);
	if (p_url) {
		sprintf_s(ip, 100, "%s",
			  extractIPAddress(std::string(p_url)).c_str());
		ndiLib->recv_free_string(recv, p_url);
	} else {
		sprintf_s(ip, 100, "127.0.0.1");
	}
	visca.connectCamera(std::string(ip), 5678);
	return visca;
};
std::vector<std::string> NDIPTZDeviceManager::getNDINames()
{
    std::vector<std::string> name_list;
    for (auto &recv : _recvs) {
          name_list.push_back(recv.first); 
    }
    return name_list;
};
std::vector<std::string> NDIPTZDeviceManager::createListOfNDINames(obs_scene_t* scene)
{
    std::vector<std::string> name_list;
    obs_scene_enum_items(scene, [](obs_scene_t*, obs_sceneitem_t* item, void* param) -> bool {
        auto names = static_cast<std::vector<std::string>*>(param);
        obs_source_t* source = obs_sceneitem_get_source(item);
        std::string ndi_name = getNDIName(source);
        if (ndi_name == "") return true;

       
        if (!obs_source_showing(source)) return true;
        names->push_back(ndi_name);
        return true;
    }, &name_list);
    return name_list;
};

void NDIPTZDeviceManager::updateRecvInfo(const NDIlib_v4 *ndiLib, 
                    const std::vector<std::string> name_list, 
                    std::map<std::string, recv_info_t>& recvs)
{
    bool changed = false;
    for (const std::string& ndi_name : name_list) {
         auto it = recvs.find(ndi_name);
        if (it == recvs.end()) {
            NDIlib_recv_instance_t recv = getRecv(ndiLib, ndi_name);
            ViscaAPI visca = getViscaAPI(ndiLib, recv);
	        recv_info_t recv_info = {};
	        recv_info.recv = recv;
		    recv_info.visca = visca;
            recvs[ndi_name] = recv_info;
            changed = true;
        }
    }
    if (changed) {
        for (const auto& callback : _recvsChangedCallbacks) {
            if (callback) {  // Check if the callback is valid
                callback(recvs);
            }
        }
    }
};