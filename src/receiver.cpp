#include "receiver.h"
#include <regex>

std::string Receiver::getDeviceName(const obs_source_t *source)
{
	ReceiverType type = getReceiverType(source);
	switch (type) {
	case ReceiverType::NDI: {
		obs_data_t *data = obs_source_get_settings(source);
		if (obs_data_get_bool(data, "ndi_ptz")) {
			return obs_data_get_string(data, "ndi_source_name");
		}
		return "";
	}
	case ReceiverType::WebCam: {
		obs_data_t *data = obs_source_get_settings(source);
		std::string video_device_id =
			obs_data_get_string(data, "video_device_id");
		// Find the position of the colon
		size_t colon_pos = video_device_id.find(':');

		// Extract the substring before the colon
		std::string device_name =
			(colon_pos != std::string::npos)
				? video_device_id.substr(0, colon_pos)
				: video_device_id; // If no colon is found, return the full string
		// Remove leading and trailing whitespace
		device_name.erase(device_name.begin(),
				  std::find_if(device_name.begin(),
					       device_name.end(),
					       [](unsigned char ch) {
						       return !std::isspace(ch);
					       }));
		return device_name;
	}
	}
	return "";
};
Receiver::Receiver()
{
	device_name = "";
	source = nullptr;
	recv = nullptr;
	visca_supported = false;
	visca = new ViscaAPI();
};
Receiver::ReceiverType Receiver::getReceiverType(const obs_source_t *source)
{
	std::string id = obs_source_get_id(source);
	if (id == "ndi_source") {
		return ReceiverType::NDI;
	}
	if (id == "dshow_input") {
		return ReceiverType::WebCam;
	}
	return ReceiverType::NotSupported;
};
void Receiver::connect(obs_source_t *source, ReceiverType rtype, std::string IP,
		       int port)
{
	auto device_name = getDeviceName(source);
	switch (rtype) {
		case Receiver::ReceiverType::NDI: {

			if (getReceiverType(source) == ReceiverType::NDI)
				this->recv = connectRecv(device_name);
			if (this->recv == nullptr) {
				blog(LOG_ERROR,
					 "[patizo] Failed to connect to NDI source: %s",
					 device_name.c_str());
				break;
			}
			auto status = visca->connectCamera(IP,
							   port, false, false);
			if (status != VOK) {
				blog(LOG_ERROR,
				     "[patizo] Failed to connect to camera. Error code: %d",
				     status);
			} else {
				blog(LOG_INFO,
				     "[patizo] Connected to camera: %s",
				     device_name.c_str());
			}
			break;
		}

		case Receiver::ReceiverType::WebCam: {
			auto status =
				visca->connectCamera(IP, port, true, true);
			if (status != VOK) {
				blog(LOG_ERROR,
				     "[patizo] Failed to connect to camera. Error code: %d",
				     status);
			} else {
				blog(LOG_INFO,
				     "[patizo] Connected to camera: %s",
				     device_name.c_str());
			}
			break;
		}
		default:
			break;
	}

	visca_supported = (this->visca->connectionStatus() == VOK);
	this->device_name = device_name;
	source = obs_source_get_ref(source);
};

NDIlib_recv_instance_t Receiver::connectRecv(const std::string &device_name)
{
	NDIlib_recv_instance_t recv = nullptr;
	NDIlib_source_t selected_source;
	selected_source.p_ndi_name = device_name.data();

	NDIlib_recv_create_v3_t recv_create_desc;
	recv_create_desc.source_to_connect_to = selected_source;
	recv_create_desc.color_format = NDIlib_recv_color_format_RGBX_RGBA;
	recv_create_desc.bandwidth = NDIlib_recv_bandwidth_metadata_only;

	recv = g_ndiLib->recv_create_v3(&recv_create_desc);
	return recv;
};

void Receiver::disconnectRecv(NDIlib_recv_instance_t recv)
{
	g_ndiLib->recv_destroy(recv);
};
std::string Receiver::extractIPAddress(const std::string &str)
{
	std::regex ip_regex(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}))");
	std::smatch match;
	if (std::regex_search(str, match, ip_regex)) {
		return match[0];
	}
	return "";
};
ViscaAPI *Receiver::getViscaAPI(NDIlib_recv_instance_t recv)
{
	ViscaAPI *visca = new ViscaAPI();
	char ip[100];
	const char *p_url = g_ndiLib->recv_get_web_control(recv);
	if (p_url) {
		snprintf(ip, 100, "%s",
			 extractIPAddress(std::string(p_url)).c_str());
		g_ndiLib->recv_free_string(recv, p_url);
	} else {
		snprintf(ip, 100, "192.168.1.173");
	}
	auto status = visca->connectCamera(std::string(ip), 52381, true, true);

	blog(LOG_INFO, "[patizo] Visca IP: %s, status: %d", ip, status);
	return visca;
};
