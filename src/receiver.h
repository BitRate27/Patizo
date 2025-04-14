#pragma once
#include "plugin-main.h"

typedef std::function<void()> ReceiverChangedCallback;
class Receiver {
public:
	Receiver();
	void connect(obs_source_t *source);
	std::string device_name;
	obs_source_t *source;
	NDIlib_recv_instance_t recv;
	bool visca_supported;
	ViscaAPI *visca;
	enum class ReceiverType { NDI, WebCam, NotSupported };
	ReceiverType getReceiverType(const obs_source_t *source);
	std::string getDeviceName(const obs_source_t *source);

private:
	NDIlib_recv_instance_t connectRecv(const std::string &ndi_name);

	void disconnectRecv(NDIlib_recv_instance_t recv);
	
	std::string extractIPAddress(const std::string &str);
	ViscaAPI *getViscaAPI(NDIlib_recv_instance_t recv);
};