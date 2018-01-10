#include <cstdlib>
#include <cstdio>
#include <sstream>
#include "logger.h"
#include "hue/hub.h"
#include "hue/light.h"
#include "hue/task.h"
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

bool HubDevice::update(const std::string &id, const std::string &ip, const std::string &name) {
	mID = id;
	mIp = ip;
	mName = name;

	HueConfigSection* configSection = mConfig.getSection("Hub", "id", id);
	if(configSection != NULL) {
		mUser = configSection->value("user");
	}

	if(!updateLights()) {
		return false;
	}

	if(!updateTasks()) {
		return false;
	}

	return true;
}

bool HubDevice::updateLights() {
	if(!isAuthorized()) {
		return false;
	}

	std::vector<std::string> lightIds;

	json_object* lightsObj;
	if(downloadJson("http://" + mIp + "/api/" + mUser + "/lights", &lightsObj)) {
		json_object_object_foreach(lightsObj, key, val) {
			json_object *idObj;
			if(!json_object_object_get_ex(val, "uniqueid", &idObj)) {
				continue;
			}

			std::string id = json_object_get_string(idObj);

			bool found = false;
			for(std::vector<HueLight*>::iterator it = mLights.begin(); it != mLights.end(); ++it) {
				if(*(*it) == id) {
					(*it)->update(val, atoi(key));
					found = true;
					break;
				}
			}

			lightIds.push_back(id);

			if(found) {
				continue;
			}

			HueLight* light = new HueLight(val, atoi(key));
			if(!light->valid()) {
				Logger::error() << "Light " << id << " is not valid\n";
			}

			mLights.push_back(light);
		}

		json_object_put(lightsObj);
	} else {
		return false;
	}

	for(uint32_t i = 0; i < mLights.size(); i++) {
		bool found = false;
		for(std::vector<std::string>::iterator it = lightIds.begin(); it != lightIds.end(); ++it) {
			if(*(mLights[i]) == (*it)) {
				found = true;
			}
		}

		if(!found) {
			delete mLights[i];
			mLights.erase(mLights.begin() + i);
			i--;
		}
	}

	return true;
}

bool HubDevice::updateTasks() {
	// Look for new tasks.
	const std::vector<HueConfigSection*> sections = mConfig.getSections("Task", "hub", mID);
	for(std::vector<HueConfigSection*>::const_iterator it = sections.begin(); it != sections.end(); ++it) {
		bool shouldAdd = true;
		for(std::vector<HueTask*>::const_iterator taskIt = mTasks.begin(); taskIt != mTasks.end(); ++taskIt) {
			if(*(*taskIt) == (*it)->value("id")) {
				(*taskIt)->update(mConfig, *(*it));

				shouldAdd = false;
				break;
			}
		}

		if(!shouldAdd) {
			continue;
		}

		HueTask* task = HueTask::fromConfig(mConfig, *(*it), *this);
		if(task == NULL) {
			continue;
		}
		if(!task->valid()) {
			delete task;
			continue;
		}

		mTasks.push_back(task);
	}

	// Look for removed tasks.
	for(uint32_t i = 0; i < mTasks.size(); i++) {
		bool found = false;
		for(std::vector<HueConfigSection*>::const_iterator it = sections.begin(); it != sections.end(); ++it) {
			if(*mTasks.at(i) == (*it)->value("id")) {
				found = true;
			}
		}

		// Couldn't find it in the config file, remove it!
		if(!found) {
			delete mTasks[i];
			mTasks.erase(mTasks.begin() + i);
			i--;
		}
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