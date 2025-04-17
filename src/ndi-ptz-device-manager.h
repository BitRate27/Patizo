#pragma once
#include <map>
#include <string>
#include <functional>
#include <vector>
#include <obs.h>
#include "receiver.h"


typedef std::function<void()> RecvsChangedCallback;
std::vector<obs_source_t *> getSourcesInScene(obs_source_t *scene_source);

class NDIPTZDeviceManager {
public:
	NDIPTZDeviceManager() : _current(""), _recvs() {};
    void init();
    ~NDIPTZDeviceManager();

    Receiver *getRecvInfo(const std::string &ndi_name);
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
    std::map<std::string, Receiver*> _recvs;
    std::vector<RecvsChangedCallback> _recvsChangedCallbacks;
    std::string _current = "";
    // Add the private member variable
    PreviewStatus _currentPreviewStatus = PreviewStatus::NotSupported;
    // Helper methods
    void updateRecvInfo(const std::vector<obs_source_t *> source_list, 
                    std::map<std::string, Receiver*>& recvs);
    void notifyCallbacks()
    {
	    for (const auto &callback : _recvsChangedCallbacks) {
		    if (callback) { // Check if the callback is valid
			    callback();
		    }
	    }
    };
    void closeAllConnections();
    std::vector<obs_source_t*> createListOfNDISources(obs_scene_t *scene);
};
