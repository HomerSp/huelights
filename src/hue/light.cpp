#include <sstream>
#include <cstdio>
#include "hue/light.h" 
#include "connection.h"

HueLightState::HueLightState(HueLightState* state)
	: mValid(true),
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
	json_object *strObj;

	JSON_GET(stateObj, "on", boolean, mOn);
	JSON_GET(stateObj, "bri", int, mBrightness);
	JSON_GET(stateObj, "alert", string, mAlert);
	JSON_GET(stateObj, "reachable", boolean, mReachable);

	mValid = true;
}

HueLight::HueLight(json_object* lightObj, int index)
	: mValid(false),
	mNewState(NULL),
	mIndex(index)
{	
	json_object *strObj;

	if(!json_object_object_get_ex(lightObj, "type", &strObj)) {
		return;
	}

	mType = json_object_get_string(strObj);

	if(!json_object_object_get_ex(lightObj, "name", &strObj)) {
		return;
	}
	mName = json_object_get_string(strObj);

	json_object_object_get_ex(lightObj, "modelid", &strObj);
	mModel = json_object_get_string(strObj);

	json_object_object_get_ex(lightObj, "manufacturername", &strObj);
	mManufacturer = json_object_get_string(strObj);

	json_object_object_get_ex(lightObj, "uniqueid", &strObj);
	mID = json_object_get_string(strObj);

	json_object_object_get_ex(lightObj, "swversion", &strObj);
	mVersion = json_object_get_string(strObj);

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
		fprintf(stderr, "Failed to set some parameters\n");
	}

	json_object_put(output);
	json_object_put(obj);

	return true;
}

std::string HueLight::toJson() const {
	json_object* obj = json_object_new_object();
	json_object_object_add(obj, "index", json_object_new_int(mIndex));
	json_object_object_add(obj, "id", json_object_new_string(mID.c_str()));
	json_object_object_add(obj, "name", json_object_new_string(mName.c_str()));
	json_object_object_add(obj, "on", json_object_new_boolean(state()->on()));
	json_object_object_add(obj, "brightness", json_object_new_int(state()->brightness()));
	json_object_object_add(obj, "reachable", json_object_new_boolean(state()->reachable()));

	std::string ret = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
	json_object_put(obj);
	return ret;
}

std::string HueLight::toString() const {
	std::ostringstream ret;
	ret << "[Light]\n"
		<< "index=" << mIndex << "\n"
	 	<< "id=" << mID << "\n"
		<< "name=" << mName << "\n"
		<< "on=" << ((state()->on())?"true":"false") << "\n"
		<< "brightness=" << state()->brightness() << "\n"
		<< "reachable=" << ((state()->reachable())?"true":"false") << "\n";

	return ret.str();
}