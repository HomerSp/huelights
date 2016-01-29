#ifndef INCLUDES_HUE_TASKS_TASK_TIME_H
#define INCLUDES_HUE_TASKS_TASK_TIME_H

#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include "hue/task.h"

class HueTask;

class HueTaskTime : public HueTask {
public:
	enum Method {
		MethodDateTime = 0
	};

	HueTaskTime(const HueConfigSection &config, const HubDevice& device);

	virtual bool execute(bool& fatalError);

protected:
	virtual void toJsonInt(json_object* obj) const;
	virtual void toStringInt(std::ostringstream& s) const;
	
}; 

#endif //INCLUDES_HUE_TASKS_TASK_TIME_H
