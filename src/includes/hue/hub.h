#ifndef INCLUDES_HUE_HUB_H
#define INCLUDES_HUE_HUB_H

#include <iostream>
#include <vector>
#include <json-c/json.h>
#include "config.h"

class HueLight;
class HueTask;

class HubDevice {
public:
	HubDevice(const std::string &id, const std::string &ip, const std::string &name, HueConfig& config);
	~HubDevice();

	bool authorize(bool& retry);

	bool updateLights();
	bool updateTasks();

	const std::vector<HueLight*> &lights() const {
		return mLights;
	}

	const std::vector<HueTask*> &tasks() const {
		return mTasks;
	}

	HueConfig& config() {
		return mConfig;
	}

	HueLight* light(const std::string& id) const;
	HueTask* task(const std::string& id) const;

	bool isAuthorized() const {
		return mUser.size() > 0;
	}

	const std::string &ip() const {
		return mIp;
	} 

	const std::string &id() const {
		return mID;
	}

	const std::string &name() const {
		return mName;
	}

	const std::string &user() const {
		return mUser;
	}

	json_object* toJson() const;
	std::string toString() const;

private:
	HueConfig& mConfig;

	std::string mID;
	std::string mIp;
	std::string mName;

	std::string mUser;

	std::vector<HueLight*> mLights;
	std::vector<HueTask*> mTasks;

};

#endif //INCLUDES_HUE_HUB_H

