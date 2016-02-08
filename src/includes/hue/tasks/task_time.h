#ifndef INCLUDES_HUE_TASKS_TASK_TIME_H
#define INCLUDES_HUE_TASKS_TASK_TIME_H

#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>
#include "hue/task.h"

class HueTask;

class HueTaskTime : public HueTask {
public:
	enum Method {
		MethodNone = 0,
		MethodDateTime,
	};

	HueTaskTime(const HueConfigSection &taskConfig, const HueConfigSection &stateConfig, const HueConfigSection &triggerConfig, const HubDevice& device);

	virtual bool execute(bool& fatalError);

protected:
	virtual void toJsonInt(json_object* obj) const;
	virtual void toStringInt(std::ostringstream& s) const;
	
private:
	static std::map<std::string, Method> sSupportedMethods;

	Method mTaskMethod;
	struct tm mTime;
}; 

#endif //INCLUDES_HUE_TASKS_TASK_TIME_H
