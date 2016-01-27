#ifndef INCLUDES_HUE_HUB_H
#define INCLUDES_HUE_HUB_H

#include <iostream>
#include <vector>
#include "config.h"
#include "light.h"

class HueLight;

class HubDevice {
public:
	HubDevice(const std::string &id, const std::string &ip, const std::string &name, const HueConfig& config);

	bool authorize(bool& retry, HueConfig& config);
	bool updateLights();

	const std::vector<HueLight*> &lights() const {
		return mLights;
	}

	HueLight* light(const std::string& id);

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

	std::string toJson() const;
	std::string toString() const;

private:
	std::string mID;
	std::string mIp;
	std::string mName;

	std::string mUser;

	std::vector<HueLight*> mLights;

};

#endif //INCLUDES_HUE_HUB_H

