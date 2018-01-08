#include <sstream>
#include <cstdlib>
#include <iomanip>
#include "hue/task.h" 
#include "hue/tasks/task_time.h"
#include "utils.h"

HueTask::HueTask(const HueConfig& config, const HueConfigSection &taskConfig, const HubDevice& device)
	: mValid(false),
	mDevice(device),
	mEnabled(false),
	mStateToggle(false)
{
	// Type has to be set, or the update method won't work.
	if(!taskConfig.hasKey("type")) {
		return;
	}

	mType = taskConfig.value("type");
}

HueTask* HueTask::fromConfig(const HueConfig& config, const HueConfigSection &taskConfig, const HubDevice& device) {
	if(!taskConfig.hasKey("type")) {
		return NULL;
	}

	std::string type = taskConfig.value("type");

	#define GET_TASK_TYPE(t, c) do {\
		if(type == t) {\
			ret = new c(config, taskConfig, device);\
		} \
	} while(0)

	HueTask* ret = NULL;
	GET_TASK_TYPE("time", HueTaskTime);
	if(ret == NULL) {
		std::cerr << "Unknown task type " << type << "\n";

		return NULL;
	}

	if(!ret->valid()) {
		delete ret;
		return NULL;
	}

	return ret;
}

bool HueTask::update(const HueConfig& config, const HueConfigSection& taskConfig) {
	mValid = false;

	if(!taskConfig.hasKey("id") || !taskConfig.hasKey("type")) {
		return false;
	}

	std::string id = taskConfig.value("id");
	std::string type = taskConfig.value("type");

	// We can't do anything if the type has been changed...
	if(mType != type) {
		return false;
	}

	HueConfigSection *stateConfig = config.getSection("Task " + id + " State");
	if(stateConfig == NULL) {
		return false;
	}

	HueConfigSection *triggerConfig = config.getSection("Task " + id + " Trigger");
	if(triggerConfig == NULL) {
		return false;
	}

	if(!taskConfig.hasKey("id") || !taskConfig.hasKey("name") || !taskConfig.hasKey("type")) {
		return false;
	}

	mEnabled = taskConfig.boolValue("enabled", mEnabled);
	mID = taskConfig.value("id");
	mName = taskConfig.value("name");
	mType = taskConfig.value("type");

	// lights should be a comma-separated list of light id's
	std::set<std::string> lightIDs;
	commaListToSet(taskConfig.value("lights"), lightIDs);

	mLights.clear();
	for(std::set<std::string>::iterator it = lightIDs.begin(); it != lightIDs.end(); ++it) {
		HueLight* light = mDevice.light(*it);
		if(light != NULL) {
			mLights.push_back(light);
		} else {
			std::cerr << "Task (" << mID << ") Failed to find light " << *it << ", please check your configuration!\n";
			return false;
		}

	}

	mState.reset();
	mStateToggle = false;
	if(stateConfig->hasKey("state")) {
		std::string state = stateConfig->value("state");
		if(state == "toggle") {
			mStateToggle = true;
		} else {
			mState.setOn(state == "on");
		}
	}
	if(stateConfig->hasKey("brightness")) {
		mState.setBrightness(stateConfig->intValue("brightness"));
	}

	if(!update(*triggerConfig)) {
		return false;
	}

	mValid = true;
	return true;
}

void HueTask::reset() {

}

void HueTask::generateID() {
	std::ostringstream ret;
	ret << std::setfill('0') << std::setw(2) << std::hex
	<< (rand() % 256) << (rand() % 256)
	<< (rand() % 256) << (rand() % 256)
	<< (rand() % 256) << (rand() % 256)
	<< (rand() % 256) << (rand() % 256);

	mID = ret.str();
}

bool HueTask::trigger() {
	for(std::vector<HueLight*>::const_iterator it = mLights.begin(); it != mLights.end(); ++it) {
		mState.copyTo((*it)->newState());

		if(mStateToggle) {
			(*it)->newState()->setOn((*it)->state()->on());
			(*it)->newState()->toggle();
		}

		(*it)->write(mDevice);
	}

	return true;
}

json_object* HueTask::toJson() const {
	json_object* obj = json_object_new_object();
	json_object_object_add(obj, "id", json_object_new_string(mID.c_str()));
	json_object_object_add(obj, "name", json_object_new_string(mName.c_str()));

	json_object* stateObj = json_object_new_object();

	if(mStateToggle) {
		json_object_object_add(stateObj, "state", json_object_new_string("toggle"));
	} else if(mState.isSet(HueLightState::StateSetPower)) {
		json_object_object_add(stateObj, "state", json_object_new_string((mState.on() ? "on" : "off")));
	}
	if(mState.isSet(HueLightState::StateSetBrightness)) {
		json_object_object_add(stateObj, "brightness", json_object_new_int(mState.brightness()));
	}
	if(mState.isSet(HueLightState::StateSetAlert)) {
		json_object_object_add(stateObj, "alert", json_object_new_string(mState.alert().c_str()));
	}

	json_object_object_add(obj, "state", stateObj);

	toJsonInt(obj);
	return obj;
}

std::string HueTask::toString() const {
	std::ostringstream s;
	s << "[Task]"
		<< "\n" << "id=" << mID
		<< "\n" << "name=" << mName
		<< "\n" << "enabled=" << ((mEnabled)?"true":"false")
		<< "\n" << "hub=" << mDevice.id()
		<< "\n" << "lights=";

	for(std::vector<HueLight*>::const_iterator it = mLights.begin(); it != mLights.end(); ++it) {
		s << (*it)->id();

		if(it + 1 != mLights.end()) {
			s << ",";
		}
	}

	s << "\n" << "type=" << mType;

	toStringInt(s);

	s << "\n\n" << "[Task " << mID << " State]";
	if(mStateToggle) {
		s << "\n" << "state=toggle";
	} else if(mState.isSet(HueLightState::StateSetPower)) {
		s << "\n" << "state=" << (mState.on() ? "on" : "off");
	}
	if(mState.isSet(HueLightState::StateSetBrightness)) {
		s << "\n" << "brightness=" << mState.brightness();
	}
	if(mState.isSet(HueLightState::StateSetAlert)) {
		s << "\n" << "alert=" << mState.alert();
	}

	return s.str();
}
