#ifndef INCLUDES_HUE_TASKS_TASK_TIME_H
#define INCLUDES_HUE_TASKS_TASK_TIME_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdlib>
#include <ctime>
#include "hue/task.h"

class HueTask;

class HueTaskTime : public HueTask {
public:
	enum Method {
		MethodNone = 0,
		MethodFixed,
		MethodRecurring,
	};
	enum SunCalculation {
		SunNone = 0,
		SunRise = 1,
		SunSet = 2,
	};

	HueTaskTime(const HueConfigSection &taskConfig, const HueConfigSection &stateConfig, const HueConfigSection &triggerConfig, const HubDevice& device);

	virtual bool execute(bool& fatalError);

	virtual void updateTrigger(time_t now, time_t* diff = NULL);

protected:
	virtual void toJsonInt(json_object* obj) const;
	virtual void toStringInt(std::ostringstream& s) const;
	
private:
	static std::map<std::string, Method> sSupportedMethods;

	Method mTaskMethod;

	std::pair<double, double> mPosition;
	int mTimeSun;
	struct tm mTime;
	std::set<uint32_t> mRepeatDays;
}; 

#endif //INCLUDES_HUE_TASKS_TASK_TIME_H
