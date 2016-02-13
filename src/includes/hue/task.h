#ifndef INCLUDES_HUE_TASK_H
#define INCLUDES_HUE_TASK_H

#include <iostream>
#include <vector>
#include "config.h"
#include "hub.h"
#include "light.h"

class HubDevice;
class HueLight;
class HueLightState;

class HueTask {
public:
	virtual ~HueTask() {}

	static HueTask* fromConfig(const HueConfig& config, const HueConfigSection &taskConfig, const HubDevice& device);

	virtual bool execute(bool& fatalError) = 0;
	virtual void updateTrigger(time_t now) = 0;

	bool update(const HueConfig& config, const HueConfigSection& taskConfig);
	virtual void reset();

	bool valid() const {
		return mValid;
	}

	bool enabled() const {
		return mEnabled;
	}

	const std::string& id() const {
		return mID;
	}
	const std::string &name() const {
		return mName;
	}

	void setEnabled(bool enabled) {
		mEnabled = enabled;
	}

	virtual json_object* toJson() const;
	virtual std::string toString() const;

	bool operator==(const std::string id) const {
		return mID == id;
	}

	bool operator==(const HueTask& other) const {
		return mID == other.mID;
	}

protected:
	HueTask(const HueConfig& config, const HueConfigSection &taskConfig, const HubDevice& device);

	virtual void toJsonInt(json_object* obj) const = 0;
	virtual void toStringInt(std::ostringstream& s) const = 0;

	void generateID();

	bool trigger();

	virtual bool update(const HueConfigSection& triggerConfig) = 0;

	bool mValid;

private:
	const HubDevice& mDevice;

	bool mEnabled;
	std::string mID;
	std::string mName;
	std::string mType;

	std::vector<HueLight*> mLights;

	HueLightState mState;
	bool mStateToggle;
};

#endif //INCLUDES_HUE_TASK_H
