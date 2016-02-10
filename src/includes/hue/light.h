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
	enum StateSet {
		StateSetPower = 0x01,
		StateSetBrightness = 0x02,
		StateSetAlert = 0x04,
	};

	HueLightState();
	HueLightState(HueLightState* state);
	HueLightState(json_object* stateObj);

	bool valid() const {
		return mValid;
	}

	bool isSet(StateSet state) const {
		return (mStates & state) != 0;
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
		mStates |= StateSetPower;

	}
	void toggle() {
		setOn(!mOn);
	}
	void setBrightness(int brightness) {
		mBrightness = brightness;
		mStates |= StateSetBrightness;
	}
	void setAlert(std::string alert) {
		mAlert = alert;
		mStates |= StateSetAlert;
	}

	bool operator==(const HueLightState& other) const {
		return mStates == other.mStates;
	}

	bool operator!=(const HueLightState& other) const {
		return !(*this == other);
	}

private:
	bool mValid;

	int mStates;

	bool mOn;
	int mBrightness;
	std::string mAlert;
	bool mReachable;
};

class HueLight {
public:
	HueLight(json_object* lightObj, int index);
	~HueLight();

	bool write(const HubDevice& device);

	bool valid() const {
		return mValid;
	}

	const std::string& id() const {
		return mID;
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
