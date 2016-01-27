#include <cstdlib>
#include <cstdio>
#include <sstream>
#include "hue/hub.h"
#include "connection.h"

HubDevice::HubDevice(const std::string &id, const std::string &ip, const std::string &name, const HueConfig& config)
	: mID(id),
	mIp(ip),
	mName(name),
	mUser("")
{
	HueConfigSection* configSection = config.getSection("Hub", "id", id);
	if(configSection != NULL) {
		mUser = configSection->value("user");
	}
}

HueLight* HubDevice::light(const std::string& id) {
	for(size_t i = 0; i < mLights.size(); i++) {
		if(*mLights[i] == id) {
			return mLights[i];
		}
	}		

	return NULL;
}

bool HubDevice::authorize(bool& retry, HueConfig& config) {
	bool ret = false;
	retry = false;

	json_object* inputObj = json_object_new_object();
	json_object_object_add(inputObj, "devicetype", json_object_new_string("huelights#openwrt"));

	json_object* authObj;
	if(!postJson("http://" + mIp + "/api", inputObj, &authObj)) {
		json_object_put(inputObj);
		return false;
	}

	if(json_object_array_length(authObj) == 1) {
		json_object *errorObj;
		if(json_object_object_get_ex(json_object_array_get_idx(authObj, 0), "error", &errorObj)) {
			json_object* errorTypeObj;
			if(json_object_object_get_ex(errorObj, "type", &errorTypeObj)) {
				fprintf(stderr, "Error type: %d\n", json_object_get_int(errorTypeObj));
				if(json_object_get_int(errorTypeObj) == 101) {
					retry = true;
				}
			}
		}

		json_object* successObj;
		if(json_object_object_get_ex(json_object_array_get_idx(authObj, 0), "success", &successObj)) {
			json_object* usernameObj;
			if(json_object_object_get_ex(successObj, "username", &usernameObj)) {
				mUser = json_object_get_string(usernameObj);
				ret = true;
			}
		}
	}

	json_object_put(authObj);
	json_object_put(inputObj);

	HueConfigSection* section = config.getSection("Hub", "id", mID);
	if(section == NULL) {
		section = config.newSection("Hub");
	}

	section->setValue("id", mID);
	section->setValue("name", mName);
	section->setValue("user", mUser);

	return ret;
}

bool HubDevice::updateLights() {
	if(!isAuthorized()) {
		return false;
	}

	if(mLights.size() > 0) {
		for(size_t i = 0; i < mLights.size(); i++) {
			delete mLights[i];
		}

		mLights.clear();
	}

	json_object* lightsObj;
	if(downloadJson("http://" + mIp + "/api/" + mUser + "/lights", &lightsObj)) {
		json_object_object_foreach(lightsObj, key, val) {
			mLights.push_back(new HueLight(val, atoi(key)));
		}

		json_object_put(lightsObj);
	} else {
		return false;
	}

	return true;
}

std::string HubDevice::toJson() const {
	json_object* obj = json_object_new_object();
	json_object_object_add(obj, "id", json_object_new_string(mID.c_str()));
	json_object_object_add(obj, "ip", json_object_new_string(mIp.c_str()));
	json_object_object_add(obj, "name", json_object_new_string(mName.c_str()));
	if(isAuthorized()) {
		json_object_object_add(obj, "user", json_object_new_string(mUser.c_str()));
	}

	std::string ret = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
	json_object_put(obj);
	return ret;
}

std::string HubDevice::toString() const {
	std::ostringstream ret;
	ret << "[Hub]\n"
	 	<< "id=" << mID << "\n"
	 	<< "ip=" << mIp << "\n"
		<< "name=" << mName << "\n";

	if(isAuthorized()) {
		ret << "user=" + mUser + "\n";
	}

	return ret.str();
}