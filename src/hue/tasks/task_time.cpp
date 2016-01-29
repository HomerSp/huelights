#include "hue/tasks/task_time.h" 

HueTaskTime::HueTaskTime(const HueConfigSection &config, const HubDevice& device)
	: HueTask(config, device) {

}

bool HueTaskTime::execute(bool& fatalError) {
	return true;
}

void HueTaskTime::toJsonInt(json_object* obj) const {

}

void HueTaskTime::toStringInt(std::ostringstream& s) const {

}
