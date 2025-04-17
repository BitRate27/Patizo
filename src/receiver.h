#pragma once
#include "plugin-main.h"

typedef std::function<void()> ReceiverChangedCallback;
class Receiver {
public:	
	enum class ReceiverType { NDI, WebCam, NotSupported };
	Receiver();
	void connect(obs_source_t *source, ReceiverType rtype, std::string IP, int port);
	std::string device_name;
	obs_source_t *source;
	NDIlib_recv_instance_t recv;
	bool visca_supported;
	ViscaAPI *visca;

	ReceiverType getReceiverType(const obs_source_t *source);
	std::string getDeviceName(const obs_source_t *source);

private:
	NDIlib_recv_instance_t connectRecv(const std::string &ndi_name);

	void disconnectRecv(NDIlib_recv_instance_t recv);
	
	std::string extractIPAddress(const std::string &str);
	ViscaAPI *getViscaAPI(NDIlib_recv_instance_t recv);
};
