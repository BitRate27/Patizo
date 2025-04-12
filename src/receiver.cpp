#include "receiver.h"
#include <regex>

std::string getNDIName(const obs_source_t *source)
{
	std::string id = obs_source_get_id(source);
	if (id != "ndi_source")
		return "";
	obs_data_t *data = obs_source_get_settings(source);
	if (obs_data_get_bool(data, "ndi_ptz")) {
		return obs_data_get_string(data, "ndi_source_name");
	}
	return "";
};
Receiver::Receiver() {
	ndi_name = "";
	source = nullptr;
	recv = nullptr;
	visca_supported = false;
	visca = nullptr;
};
void Receiver::connect(obs_source_t *source) {
	auto ndi_name = getNDIName(source);
	this->recv = connectRecv(ndi_name);
	visca = getViscaAPI(this->recv);
	visca_supported =
		(this->visca->connectionStatus() == VOK);
	this->ndi_name = ndi_name;
	source = obs_source_get_ref(source);
};

NDIlib_recv_instance_t Receiver::connectRecv(
					const std::string &ndi_name)
{
	NDIlib_recv_instance_t recv = nullptr;
	NDIlib_source_t selected_source;
	selected_source.p_ndi_name = ndi_name.data();

	NDIlib_recv_create_v3_t recv_create_desc;
	recv_create_desc.source_to_connect_to = selected_source;
	recv_create_desc.color_format =
		NDIlib_recv_color_format_RGBX_RGBA;
	recv_create_desc.bandwidth =
		NDIlib_recv_bandwidth_metadata_only;

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
	auto status = visca->connectCamera(std::string(ip), 52381, true,
						true);

	blog(LOG_INFO, "[patizo] Visca IP: %s, status: %d", ip, status);
	return visca;
};
