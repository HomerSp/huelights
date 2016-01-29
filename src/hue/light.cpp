#include <sstream>
#include <cstdio>
#include "hue/light.h" 
#include "connection.h"

HueLightState::HueLightState() :
	mValid(true),
	mOn(false),
	mBrightness(-1),
	mAlert(""),
	mReachable(false)
{

}

HueLightState::HueLightState(HueLightState* state) :
	mValid(true),
	mOn(state->mOn),
	mBrightness(state->mBrightness),
	mAlert(state->mAlert),
	mReachable(state->mReachable)
{

}

#define JSON_GET(o, s, t, v) {\
	json_object *strObj;\
	if(!json_object_object_get_ex(o, s, &strObj)) {\
		return;\
	}\
	v = json_object_get_##t(strObj);\
}

HueLightState::HueLightState(json_object* stateObj)
	: mValid(false)
{
	JSON_GET(stateObj, "on", boolean, mOn);
	JSON_GET(stateObj, "bri", int, mBrightness);
	JSON_GET(stateObj, "alert", string, mAlert);
	JSON_GET(stateObj, "reachable", boolean, mReachable);

	mValid = true;
}

void HueLightState::copyTo(HueLightState* other) {
	if(other == NULL) {
		return;
	}

	// Don't copy the on state
	if(mBrightness >= 0) {
		other->setBrightness(mBrightness);
	}
	if(mAlert.length() > 0) {
		other->setAlert(mAlert);
	}
}

HueLight::HueLight(json_object* lightObj, int index)
	: mValid(false),
	mNewState(NULL),
	mIndex(index)
{	
	JSON_GET(lightObj, "type", string, mType);
	JSON_GET(lightObj, "name", string, mName);
	JSON_GET(lightObj, "modelid", string, mModel);
	JSON_GET(lightObj, "manufacturername", string, mManufacturer);
	JSON_GET(lightObj, "uniqueid", string, mID);
	JSON_GET(lightObj, "swversion", string, mVersion);

	json_object* stateObj;
	json_object_object_get_ex(lightObj, "state", &stateObj);
	mState = new HueLightState(stateObj);

	mValid = true;
}

bool HueLight::write(const HubDevice& device) {
	if(*state() == *newState()) {
		return false;
	}

	json_object* obj = json_object_new_object();
	if(state()->on() != newState()->on()) {
		json_object_object_add(obj, "on", json_object_new_boolean(newState()->on()));
	}
	if(state()->brightness() != newState()->brightness()) {
		json_object_object_add(obj, "bri", json_object_new_int(newState()->brightness()));
	}

	std::ostringstream url;
	url << "http://"
		<< device.ip()
		<< "/api/"
		<< device.user()
		<< "/lights/"
		<< mIndex
		<< "/state";

	//url << "http://192.168.1.101";

	json_object* output;
	if(!putJson(url.str(), obj, &output)) {
		json_object_put(obj);
		return false;
	}

	bool success = true;
	for(int i = 0; i < json_object_array_length(output); i++) {
		json_object* successObj;
		if(!json_object_object_get_ex(json_object_array_get_idx(output, i), "success", &successObj)) {
			success = false;
		}
	}

	if(!success) {
		std::cerr << "Failed to set some parameters for light " << mID << "\n";
	}

	json_object_put(output);
	json_object_put(obj);

	return true;
}

json_object* HueLight::toJson() const {
	json_object* obj = json_object_new_object();
	json_object_object_add(obj, "index", json_object_new_int(mIndex));
	json_object_object_add(obj, "id", json_object_new_string(mID.c_str()));
	json_object_object_add(obj, "name", json_object_new_string(mName.c_str()));
	json_object_object_add(obj, "on", json_object_new_boolean(state()->on()));
	json_object_object_add(obj, "brightness", json_object_new_int(state()->brightness()));
	json_object_object_add(obj, "reachable", json_object_new_boolean(state()->reachable()));
	return obj;
}

std::string HueLight::toString() const {
	std::ostringstream ret;
	ret << "[Light]\n"
		<< "index=" << mIndex << "\n"
	 	<< "id=" << mID << "\n"
		<< "name=" << mName << "\n"
		<< "on=" << ((state()->on())?"true":"false") << "\n"
		<< "brightness=" << state()->brightness() << "\n"
		<< "reachable=" << ((state()->reachable())?"true":"false");

	return ret.str();
}