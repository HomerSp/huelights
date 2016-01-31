#include <sstream>
#include "hue/tasks/task_time.h" 

HueTaskTime::HueTaskTime(const HueConfigSection &config, const HubDevice& device)
	: HueTask(config, device),
	mTaskMethod(HueTaskTime::MethodNone)
{

	std::string m = HueTask::method();
	if(m == "datetime") {
		mTaskMethod = HueTaskTime::MethodDateTime;
	}

	if(mTaskMethod == HueTaskTime::MethodNone) {
		HueTask::mValid = false;
		return;
	}

    if (!strptime(config.value("time").c_str(), "%Y-%m-%d %H:%M", &mTime)) {
		HueTask::mValid = false;
		return;
    }
}

bool HueTaskTime::execute(bool& fatalError) {
	return true;
}

void HueTaskTime::toJsonInt(json_object* obj) const {

}

void HueTaskTime::toStringInt(std::ostringstream& s) const {
	s << "\ntime=" << (1900 + mTime.tm_year) << "-" << (1 + mTime.tm_mon) << "-" << mTime.tm_mday << " " << mTime.tm_hour << ":" << mTime.tm_min;
}
