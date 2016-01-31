#include <sstream>
#include <cstdlib>
#include <iomanip>
#include "hue/task.h" 
#include "hue/tasks/task_time.h"

HueTask::HueTask(const HueConfigSection &config, const HubDevice& device)
	: mValid(false),
	mDevice(device),
	mID(""),
	mName("")
{
	if(!config.hasKey("id") || !config.hasKey("name")) {
		return;
	}

	mID = config.value("id");
	mName = config.value("name");
	mType = config.value("type");
	mMethod = config.value("method");

	std::string lights = config.value("lights");
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

	mValid = true;
}

HueTask* HueTask::fromConfig(const HueConfigSection &config, const HubDevice& device) {
	if(!config.hasKey("type")) {
		return NULL;
	}

	#define GET_TASK_TYPE(t, c) do {\
		if(type == t) {\
			return new c(config, device);\
		} \
	} while(0)

	std::string type = config.value("type");
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

json_object* HueTask::toJson() const {
	json_object* obj = json_object_new_object();
	json_object_object_add(obj, "id", json_object_new_string(mID.c_str()));
	json_object_object_add(obj, "name", json_object_new_string(mName.c_str()));

	toJsonInt(obj);
	return obj;
}

std::string HueTask::toString() const {
	std::ostringstream s;
	s << "[Task]" << "\n"
		<< "id=" << mID << "\n"
		<< "name=" << mName;

	toStringInt(s);
	return s.str();
}
