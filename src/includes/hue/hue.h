#ifndef INCLUDES_HUE_H
#define INCLUDES_HUE_H

#include <vector>
#include "hue/config.h"
#include "hue/light.h"
#include "hue/hub.h"

class Hue {
public:
	static HubDevice* getHubDevice(const std::string& id, const HueConfig& config);
	static bool getHubDevices(std::vector<HubDevice *> &devices, const HueConfig& config);
};

#endif //INCLUDES_HUE_H
