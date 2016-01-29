#ifndef INCLUDES_HUE_LIGHT_H
#define INCLUDES_HUE_LIGHT_H

#include <iostream>
#include <vector>
#include <json-c/json.h>
#include "config.h"
#include "hub.h"

class HubDevice;

class HueLightState {
public:
	HueLightState();
	HueLightState(HueLightState* state);
	HueLightState(json_object* stateObj);

	bool valid() const {
		return mValid;
	}

	void copyTo(HueLightState* other);

	bool on() const {
		return mOn;
	}
	int brightness() const {
		return mBrightness;
	}
	const std::string &alert() const {
		return mAlert;
	}
	bool reachable() const {
		return mReachable;
	}

	void setOn(bool on = true) {
		mOn = on;
	}
	void toggle() {
		mOn = !mOn;
	}
	void setBrightness(int brightness) {
		mBrightness = brightness;
	}
	void setAlert(std::string alert) {
		mAlert = alert;
	}

	bool operator==(const HueLightState& other) const {
		return mOn == other.mOn && mBrightness == other.mBrightness;
	}

	bool operator!=(const HueLightState& other) const {
		return !(*this == other);
	}

private:
	bool mValid;

	bool mOn;
	int mBrightness;
	std::string mAlert;
	bool mReachable;
};

class HueLight {
public:
	HueLight(json_object* lightObj, int index);

	bool write(const HubDevice& device);

	bool valid() const {
		return mValid;
	}

	const std::string &name() const {
		return mName;
	}

	const HueLightState* state() const {
		return mState;
	}

	HueLightState* newState() {
		if(mNewState == NULL) {
			mNewState = new HueLightState(mState);
		}

		return mNewState;
	}
	json_object* toJson() const;
	std::string toString() const;

	bool operator==(const std::string id) const {
		return mID == id;
	}

private:
	bool mValid;

	HueLightState* mState;
	HueLightState* mNewState;

	int mIndex;
	std::string mType;
	std::string mName;
	std::string mModel;
	std::string mManufacturer;
	std::string mID;
	std::string mVersion;
};

#endif //INCLUDES_HUE_LIGHT_H
