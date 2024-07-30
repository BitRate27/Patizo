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
    NDIlib_recv_instance_t recv;
    ViscaAPI visca;
};
typedef std::function<void()> RecvsChangedCallback;

class NDIPTZDeviceManager {
public:
	NDIPTZDeviceManager() : _current(""), _recvs(), _ndiLib(nullptr) {};
    void init(const NDIlib_v4* ndiLib);
    ~NDIPTZDeviceManager();

    recv_info_t getRecvInfo(const std::string &ndi_name);
    std::string getCurrent() const;
    void onSceneChanged();
    std::vector<std::string> getNDINames();
    size_t registerRecvsChangedCallback(RecvsChangedCallback callback);
    void unregisterRecvsChangedCallback(size_t callbackId);
private:
    std::map<std::string, recv_info_t> _recvs;
    std::vector<RecvsChangedCallback> _recvsChangedCallbacks;
    const NDIlib_v4* _ndiLib;
    std::string _current = "";

    // Helper methods
    void updateRecvInfo(const NDIlib_v4* ndiLib, 
                    const std::vector<std::string> name_list, 
                    std::map<std::string, recv_info_t>& recvs);
    std::vector<std::string> createListOfNDINames(void* scene);

    void closeAllConnections();
    std::vector<std::string> createListOfNDINames(obs_scene_t *scene);
};
