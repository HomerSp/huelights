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

	HueTaskTime(const HueConfigSection &config, const HubDevice& device);

	virtual bool execute(bool& fatalError);

protected:
	virtual void toJsonInt(json_object* obj) const;
	virtual void toStringInt(std::ostringstream& s) const;
	
private:
	Method mTaskMethod;
	struct tm mTime;
}; 

#endif //INCLUDES_HUE_TASKS_TASK_TIME_H
