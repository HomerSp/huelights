#include <sstream>
#include <cstdlib>
#include <iomanip>
#include "hue/task.h" 
#include "hue/tasks/task_time.h"

HueTask::HueTask(const HueConfigSection &taskConfig, const HueConfigSection &stateConfig, const HubDevice& device)
	: mValid(false),
	mDevice(device),
	mEnabled(false),
	mStateToggle(false)
{
	if(!taskConfig.hasKey("id") || !taskConfig.hasKey("name") || !taskConfig.hasKey("type")) {
		return;
	}

	mEnabled = taskConfig.boolValue("enabled", mEnabled);
	mID = taskConfig.value("id");
	mName = taskConfig.value("name");
	mType = taskConfig.value("type");

	// lights should be a comma-separated list of light id's
	std::string lights = taskConfig.value("lights");
	size_t bpos = 0, epos = 0;
	do {
		epos = lights.find(',', bpos);
		if(epos == std::string::npos && bpos < lights.length()) {
			epos = lights.length();
		}

		std::string lightID = lights.substr(bpos, epos-bpos);
		if(lightID.length() > 0) {
			HueLight* light = device.light(lightID);
			if(light != NULL) {
				mLights.push_back(light);
			} else {
				std::cerr << "Task (" << mID << ") Failed to find light " << lightID << ", please check your configuration!\n";
				return;
			}
		}

		bpos = epos + 1;
	} while(epos != std::string::npos && bpos < lights.length());

	if(stateConfig.hasKey("status")) {
		std::string status = stateConfig.value("status");
		if(status == "toggle") {
			mStateToggle = true;
		} else {
			mState.setOn(status == "on");
		}
	}
	if(stateConfig.hasKey("brightness")) {
		mState.setBrightness(stateConfig.intValue("brightness"));
	}

	mValid = true;
}

HueTask* HueTask::fromConfig(const HueConfig& config, const HueConfigSection &taskConfig, const HubDevice& device) {
	if(!taskConfig.hasKey("id") || !taskConfig.hasKey("type")) {
		return NULL;
	}

	std::string id = taskConfig.value("id");
	std::string type = taskConfig.value("type");

	HueConfigSection *stateConfig = config.getSection("Task State", "task", id);
	if(stateConfig == NULL) {
		return NULL;
	}

	HueConfigSection *triggerConfig = config.getSection("Task Trigger", "task", id);
	if(triggerConfig == NULL) {
		return NULL;
	}

	#define GET_TASK_TYPE(t, c) do {\
		if(type == t) {\
			return new c(taskConfig, *stateConfig, *triggerConfig, device);\
		} \
	} while(0)

	GET_TASK_TYPE("time", HueTaskTime);

	std::cerr << "Unknown task type " << type << "\n";
	return NULL;
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
		if(!mStateToggle) {
			(*it)->newState()->setOn(mState.on());
		} else {
			(*it)->newState()->toggle();
		}

		mState.copyTo((*it)->newState());
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
		json_object_object_add(stateObj, "status", json_object_new_string("toggle"));
	} else if(mState.isSet(HueLightState::StateSetPower)) {
		json_object_object_add(stateObj, "status", json_object_new_string((mState.on() ? "on" : "off")));
	}
	if(mState.isSet(HueLightState::StateSetBrightness)) {
		json_object_object_add(stateObj, "brightness", json_object_new_int(mState.brightness()));
	}
	if(mState.isSet(HueLightState::StateSetAlert)) {
		json_object_object_add(stateObj, "status", json_object_new_string(mState.alert().c_str()));
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

	s << "\n\n" << "[Task State]";
	s << "\n" << "task=" << mID;
	if(mStateToggle) {
		s << "\n" << "status=toggle";
	} else if(mState.isSet(HueLightState::StateSetPower)) {
		s << "\n" << "status=" << (mState.on() ? "on" : "off");
	}
	if(mState.isSet(HueLightState::StateSetBrightness)) {
		s << "\n" << "brightness=" << mState.brightness();
	}
	if(mState.isSet(HueLightState::StateSetAlert)) {
		s << "\n" << "alert=" << mState.alert();
	}

	return s.str();
}
