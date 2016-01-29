#include <cstdlib>
#include <cstdio>
#include <sstream>
#include "hue/hub.h"
#include "connection.h"

HubDevice::HubDevice(const std::string &id, const std::string &ip, const std::string &name, HueConfig& config)
	: mConfig(config),
	mID(id),
	mIp(ip),
	mName(name),
	mUser("")
{
	HueConfigSection* configSection = mConfig.getSection("Hub", "id", id);
	if(configSection != NULL) {
		mUser = configSection->value("user");
	}

	updateLights();
	updateTasks();
}

HubDevice::~HubDevice() {
	for(std::vector<HueLight*>::const_iterator it = mLights.begin(); it != mLights.end(); ++it) {
		delete *it;
	}

	for(std::vector<HueTask*>::const_iterator it = mTasks.begin(); it != mTasks.end(); ++it) {
		delete *it;
	}
}

HueLight* HubDevice::light(const std::string& id) const {
	for(std::vector<HueLight*>::const_iterator it = mLights.begin(); it != mLights.end(); ++it) {
		if(*(*it) == id) {
			return *it;
		}
	}

	return NULL;
}

HueTask* HubDevice::task(const std::string& id) const {
	for(std::vector<HueTask*>::const_iterator it = mTasks.begin(); it != mTasks.end(); ++it) {
		if(*(*it) == id) {
			return *it;
		}
	}

	return NULL;
}

bool HubDevice::authorize(bool& retry) {
	bool ret = false;
	retry = false;

	json_object* inputObj = json_object_new_object();
	json_object_object_add(inputObj, "devicetype", json_object_new_string("huelights#openwrt"));

	/*json_object* authObj;
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
	}*/

	mUser = "75e260da479efe32658adffb2b68dc94";
	ret = false;
	retry = true;

	//json_object_put(authObj);
	json_object_put(inputObj);

	if(ret) {
		HueConfigSection* section = mConfig.getSection("Hub", "id", mID);
		if(section == NULL) {
			section = mConfig.newSection("Hub");
		}

		section->setValue("id", mID);
		section->setValue("name", mName);
		section->setValue("user", mUser);
	}

	return ret;
}

bool HubDevice::updateLights() {
	if(!isAuthorized()) {
		return false;
	}

	for(std::vector<HueLight*>::const_iterator it = mLights.begin(); it != mLights.end(); ++it) {
		delete *it;
	}

	mLights.clear();

	json_object* lightsObj;
	if(downloadJson("http://" + mIp + "/api/" + mUser + "/lights", &lightsObj)) {
		json_object_object_foreach(lightsObj, key, val) {
			HueLight* light = new HueLight(val, atoi(key));
			if(!light->valid()) {
				fprintf(stderr, "Light is not valid\n");
			}

			mLights.push_back(light);
		}

		json_object_put(lightsObj);
	} else {
		return false;
	}

	return true;
}

bool HubDevice::updateTasks() {
	for(std::vector<HueTask*>::const_iterator it = mTasks.begin(); it != mTasks.end(); ++it) {
		delete *it;
	}

	mTasks.clear();

	const std::vector<HueConfigSection*> sections = mConfig.getSections("Task", "hub", mID);
	for(std::vector<HueConfigSection*>::const_iterator it = sections.begin(); it != sections.end(); ++it) {
		HueTask* task = HueTask::fromConfig(*(*it), *this);
		if(task == NULL || !task->valid()) {
			continue;
		}

		mTasks.push_back(task);
	}

	return true;
}

json_object* HubDevice::toJson() const {
	json_object* obj = json_object_new_object();
	json_object_object_add(obj, "id", json_object_new_string(mID.c_str()));
	json_object_object_add(obj, "ip", json_object_new_string(mIp.c_str()));
	json_object_object_add(obj, "name", json_object_new_string(mName.c_str()));
	if(isAuthorized()) {
		json_object_object_add(obj, "user", json_object_new_string(mUser.c_str()));
	}

	return obj;
}

std::string HubDevice::toString() const {
	std::ostringstream ret;
	ret << "[Hub]\n"
	 	<< "id=" << mID << "\n"
	 	<< "ip=" << mIp << "\n"
		<< "name=" << mName;

	if(isAuthorized()) {
		ret << "\n" << "user=" + mUser;
	}

	return ret.str();
}