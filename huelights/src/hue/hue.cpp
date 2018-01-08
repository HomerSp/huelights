#include <cstdlib>
#include <cstdio>
#include <json-c/json.h>
#include "hue/hue.h"
#include "connection.h"

HubDevice* Hue::getHubDevice(const std::string& deviceID, HueConfig& config) {
	HubDevice* device = NULL;

	json_object* pnpObj;
	if(downloadJson("https://www.meethue.com/api/nupnp", &pnpObj)) {
		for(int i = 0; i < json_object_array_length(pnpObj); i++) {
			json_object *idObj, *ipObj;
			json_object_object_get_ex(json_object_array_get_idx(pnpObj, i), "id", &idObj);
			json_object_object_get_ex(json_object_array_get_idx(pnpObj, i), "internalipaddress", &ipObj);

			std::string id = json_object_get_string(idObj);
			std::string ip = json_object_get_string(ipObj);

			if(id != deviceID) {
				continue;
			}

			json_object* configObj;
			if(downloadJson("http://" + ip + "/api/config", &configObj)) {
				json_object *nameObj;
				json_object_object_get_ex(configObj, "name", &nameObj);

				std::string name = json_object_get_string(nameObj);
				device = new HubDevice(id, ip, name, config);

				json_object_put(configObj);

				break;
			}
		}

		json_object_put(pnpObj);
	} else {
		return NULL;
	}

	return device;
}

bool Hue::getHubDevices(std::vector<HubDevice *> &devices, HueConfig& config) {
	std::vector<std::string> deviceIds;

	json_object* pnpObj;
	if(downloadJson("https://www.meethue.com/api/nupnp", &pnpObj)) {
		for(int i = 0; i < json_object_array_length(pnpObj); i++) {
			json_object *idObj, *ipObj;
			json_object_object_get_ex(json_object_array_get_idx(pnpObj, i), "id", &idObj);
			json_object_object_get_ex(json_object_array_get_idx(pnpObj, i), "internalipaddress", &ipObj);

			std::string id = json_object_get_string(idObj);
			std::string ip = json_object_get_string(ipObj);

			json_object* configObj;
			if(downloadJson("http://" + ip + "/api/config", &configObj)) {
				json_object *nameObj;
				json_object_object_get_ex(configObj, "name", &nameObj);

				std::string name = json_object_get_string(nameObj);

				bool found = false;
				for(std::vector<HubDevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
					if(*(*it) == id) {
						(*it)->update(id, ip, name);

						found = true;
						break;
					}
				}

				if(!found) {
					devices.push_back(new HubDevice(id, ip, name, config));
				}

				deviceIds.push_back(id);

				json_object_put(configObj);
			}
		}

		json_object_put(pnpObj);
	} else {
		return false;
	}

	// Look for removed devices.
	for(uint32_t i = 0; i < devices.size(); i++) {
		bool found = false;
		for(std::vector<std::string>::iterator newIt = deviceIds.begin(); newIt != deviceIds.end(); ++newIt) {
			if(*devices.at(i) == (*newIt)) {
				found = true;
				break;
			}
		}

		if(!found) {
			delete devices[i];
			devices.erase(devices.begin() + i);
			i--;
		}
	}

	return true;
}
