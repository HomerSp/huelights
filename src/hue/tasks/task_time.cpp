#include <sstream>
#include <unistd.h>
#include "hue/tasks/task_time.h" 

std::map<std::string, HueTaskTime::Method> HueTaskTime::sSupportedMethods;

HueTaskTime::HueTaskTime(const HueConfigSection &taskConfig, const HueConfigSection &stateConfig, const HueConfigSection &triggerConfig, const HubDevice& device)
	: HueTask(taskConfig, stateConfig, device),
	mTaskMethod(HueTaskTime::MethodNone)
{
	if(sSupportedMethods.size() == 0) {
		sSupportedMethods.insert(std::pair<std::string, HueTaskTime::Method>("fixed", HueTaskTime::MethodDateTime));
	}

	std::string m = triggerConfig.value("method");
	if(sSupportedMethods.count(m) != 1) {
		HueTask::mValid = false;
		return;
	}

	mTaskMethod = sSupportedMethods.at(m);

    if (!strptime(triggerConfig.value("time").c_str(), "%Y-%m-%d %H:%M", &mTime)) {
		HueTask::mValid = false;
		return;
    }
}

bool HueTaskTime::execute(bool& fatalError) {
	time_t now;
    time(&now);

    uint64_t diff = difftime(now, mktime(&mTime));
    if(diff >= 0 && diff < 60) {
    	std::cout << "TRIGGER!\n";
		trigger();
    }

    sleep(4);

	return true;
}

void HueTaskTime::toJsonInt(json_object* obj) const {
	char buf[80];
	strftime(buf, 80, "%Y-%m-%d %H:%M", &mTime);

	json_object* triggerObj = json_object_new_object();

	for(std::map<std::string, HueTaskTime::Method>::iterator it = sSupportedMethods.begin(); it != sSupportedMethods.end(); ++it) {
		if(it->second == mTaskMethod) {
			json_object_object_add(triggerObj, "method", json_object_new_string(it->first.c_str()));
			break;
		}
	}

	json_object_object_add(triggerObj, "time", json_object_new_string(buf));

	json_object_object_add(obj, "trigger", triggerObj);
}

void HueTaskTime::toStringInt(std::ostringstream& s) const {
	char buf[80];
	strftime(buf, 80, "%Y-%m-%d %H:%M", &mTime);

	s << "\n\n" << "[Task Trigger]";
	s << "\n" << "task=" << id();
	s << "\n" << "method=";
	switch(mTaskMethod) {
		case MethodDateTime:
			s << "datetime";
			break;
		default:
			break;
	}
	s << "\n" << "time=" << buf;
}
