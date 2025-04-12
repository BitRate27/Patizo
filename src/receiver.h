#pragma once
#include "plugin-main.h"

std::string getNDIName(const obs_source_t *source);

typedef std::function<void()> ReceiverChangedCallback;
class Receiver {
public:
	Receiver();
	void connect(obs_source_t *source);
	std::string ndi_name;
	obs_source_t *source;
	NDIlib_recv_instance_t recv;
	bool visca_supported;
	ViscaAPI *visca;

private:
	NDIlib_recv_instance_t connectRecv(const std::string &ndi_name);

	void disconnectRecv(NDIlib_recv_instance_t recv);
	
	std::string extractIPAddress(const std::string &str);
	ViscaAPI *getViscaAPI(NDIlib_recv_instance_t recv);
};