#pragma once

#include <map>
#include <string>
#include <cstddef>
#include <functional>
#include <vector>
#include <obs.h>
#include "Processing.NDI.Lib.h"
#include "ViscaAPI.h"

struct recv_info_t {
    std::string ndi_name;
	obs_source_t *source;
    NDIlib_recv_instance_t recv;
    bool visca_supported;
    ViscaAPI *visca;
};
typedef std::function<void()> RecvsChangedCallback;
std::vector<obs_source_t *> getSourcesInScene(obs_source_t *scene_source);

class NDIPTZDeviceManager {
public:
	NDIPTZDeviceManager() : _current(""), _recvs(), _ndiLib(nullptr) {};
    void init(const NDIlib_v4* ndiLib);
    ~NDIPTZDeviceManager();

    recv_info_t getRecvInfo(const std::string &ndi_name);
    NDIlib_recv_instance_t connectRecv(const NDIlib_v4 *ndiLib,
				      const std::string &ndi_name);
    void disconnectRecv(const NDIlib_v4 *ndiLib, NDIlib_recv_instance_t recv);
    std::string getCurrent() const;
    void onSceneChanged();
    std::vector<std::string> getNDINames();
    size_t registerRecvsChangedCallback(RecvsChangedCallback callback);
    void unregisterRecvsChangedCallback(size_t callbackId);
    enum class PreviewStatus {
        OK,
        OnProgram,
        NotSupported
    };
    PreviewStatus getCurrentPreviewStatus() const { 
        return _currentPreviewStatus; 
    };
private:
    std::map<std::string, recv_info_t> _recvs;
    std::vector<RecvsChangedCallback> _recvsChangedCallbacks;
    const NDIlib_v4* _ndiLib;
    std::string _current = "";
    // Add the private member variable
    PreviewStatus _currentPreviewStatus = PreviewStatus::NotSupported;
    // Helper methods
    void updateRecvInfo(const NDIlib_v4* ndiLib, 
                    const std::vector<std::string> name_list, 
                    std::map<std::string, recv_info_t>& recvs);
    void notifyCallbacks()
    {
	    for (const auto &callback : _recvsChangedCallbacks) {
		    if (callback) { // Check if the callback is valid
			    callback();
		    }
	    }
    };
    void closeAllConnections();
    std::vector<std::string> createListOfNDINames(obs_scene_t *scene);
};
