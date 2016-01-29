#ifndef INCLUDES_HUE_TASK_H
#define INCLUDES_HUE_TASK_H

#include <iostream>
#include <vector>
#include "config.h"
#include "hub.h"
#include "light.h"

class HubDevice;
class HueLight;

class HueTask {
public:
	enum Type {
		TypeTime = 0
	};

	virtual ~HueTask() {}

	static HueTask* fromConfig(const HueConfigSection &config, const HubDevice& device);

	virtual bool execute(bool& fatalError) = 0;

	bool valid() const {
		return mValid;
	}

	const std::string &name() const {
		return mName;
	}

	virtual json_object* toJson() const;
	virtual std::string toString() const;

	bool operator==(const std::string id) const {
		return mID == id;
	}

protected:
	HueTask(const HueConfigSection &config, const HubDevice& device);

	virtual void toJsonInt(json_object* obj) const = 0;
	virtual void toStringInt(std::ostringstream& s) const = 0;

	void generateID();

private:
	bool mValid;

	const HubDevice& mDevice;

	std::string mID;
	std::string mName;
	std::string mType;

	std::vector<HueLight*> mLights;
};

#endif //INCLUDES_HUE_TASK_H
