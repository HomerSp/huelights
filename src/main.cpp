#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

#include "hue/hue.h"

void printHelp() {
	fprintf(stderr, "huelights\n");
	fprintf(stderr, "	--config <file> - Specify the config file (defaults to /etc/huelights)\n");
	fprintf(stderr, "	--hub <id> - Specify which hub you want to operate on\n");
	fprintf(stderr, "	--light <id> - Specify which light you want to operate on\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "	--authorize - Authorize a new hub, if you do not specify the device you will be prompted\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "	--light-on - Turn on a light\n");
	fprintf(stderr, "	--light-off - Turn off a light\n");
	fprintf(stderr, "	--light-toggle - Toggle the state of a light\n");
	fprintf(stderr, "	--light-brightness <brightness> - Set the brightness of a light\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "	--json - Print the output below in json format\n");
	fprintf(stderr, "	--list-hubs - List the available hubs on the network\n");
	fprintf(stderr, "	--list-lights - List the lights associated with a specific hub\n");
	fprintf(stderr, "					Requires --hub to be set\n");
}

bool authorizeDevice(HubDevice* device, HueConfig& config) {
	printf("Authorizing device \"%s\"\n", device->name().c_str());
	printf("Press the link button to continue...\n");

	bool retry = true;
	while(retry) {
		if(device->authorize(retry, config)) {
			printf("Device authorized!\n");
			break;
		}

		sleep(1);
	}

	config.write();

	return true;
}

int main(int argc, char** argv) {
	std::string config = "/etc/huelights";
	std::string hubID = "";
	std::string lightID = "";

	bool authorize = false;

	bool lightOn = false, lightOff = false, lightToggle = false;
	int lightBrightness = -1;

	bool jsonOutput = false;
	bool listHubs = false, listLights = false;

	if(argc == 1) {
		printHelp();
		return -1;
	}

	for(int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if(arg == "--config") {
			if(i + 1 >= argc) {
				printHelp();
				return -1;
			}

			config = std::string(argv[i + 1]);
			i++;
		} else if(arg == "--device") {
			if(i + 1 >= argc) {
				printHelp();
				return -1;
			}

			hubID = std::string(argv[i + 1]);
			i++;
		} else if(arg == "--light") {
			if(i + 1 >= argc) {
				printHelp();
				return -1;
			}

			lightID = std::string(argv[i + 1]);
			i++;
		} else if(arg == "--authorize") {
			authorize = true;
		} else if(arg == "--light-on") {
			lightOn = true;
		} else if(arg == "--light-off") {
			lightOff = true;
		} else if(arg == "--light-toggle") {
			lightToggle = true;
		} else if(arg == "--light-brightness") {
			if(i + 1 >= argc) {
				printHelp();
				return -1;
			}

			lightBrightness = atoi(argv[i + 1]);
			i++;
		} else if(arg == "--json") {
			jsonOutput = true;
		} else if(arg == "--list-hubs") {
			listHubs = true;
		} else if(arg == "--list-lights") {
			listLights = true;
		} else {
			fprintf(stderr, "Unknown option \"%s\"\n", argv[i]);
			printHelp();
			return -1;
		}
	}

	if(listLights && hubID.size() == 0) {
		printHelp();
		return -1;
	}

	if(lightID.size() == 0 && (lightOn || lightOff || lightToggle || lightBrightness >= 0)) {
		printHelp();
		return -1;
	}

	HueConfig hueConfig(config);
	if(!hueConfig.parse()) {
		fprintf(stderr, "Failed to parse config: %s\n", config.c_str());
	}

	bool listDevices = true;
	if(hubID.size() > 0) {
		HubDevice* device = Hue::getHubDevice(hubID, hueConfig);
		if(device != NULL) {
			if(authorize) {
				if(!authorizeDevice(device, hueConfig)) {
					fprintf(stderr, "Could not authorize device\n");
				}
			} else if(listLights) {
				device->updateLights();

				const std::vector<HueLight*>& lights = device->lights();
				if(jsonOutput) {
					printf("[");

					for(size_t i = 0; i < lights.size(); i++) {
						printf("%s", lights.at(i)->toJson().c_str());

						if(i + 1 < lights.size()) {
							printf(",");
						}
					}

					printf("]");
				} else {
					for(size_t i = 0; i < lights.size(); i++) {
						printf("%s\n", lights.at(i)->toString().c_str());
					}
				}
			} else if(lightOn || lightOff || lightToggle || lightBrightness >= 0) {
				device->updateLights();

				HueLight* light = device->light(lightID);
				if(light != NULL) {
					if(lightOn) {
						light->newState()->setOn();
					} else if(lightOff) {
						light->newState()->setOn(false);
					} else if(lightToggle) {
						light->newState()->toggle();
					}

					if(lightBrightness >= 0) {
						light->newState()->setBrightness(lightBrightness);
					}

					light->write(*device);
				} else {
					fprintf(stderr, "Could not find light %s\n", lightID.c_str());
				}
			}

			listDevices = false;

			delete device;
		}
	}

	if(listDevices) {
		std::vector<HubDevice* > devices;
		if(Hue::getHubDevices(devices, hueConfig)) {
			if(authorize) {
				size_t index = -1;
				if(devices.size() > 1) {
					printf("Choose which device you want to authorize:\n");
					for(unsigned int i = 0; i < devices.size(); i++) {
						printf("[%d] %s (%s)\n", i, devices[i]->name().c_str(), devices[i]->ip().c_str());
					}

					unsigned char c = getchar();
					if(c >= '0' && c <= '9') {
						if(c - (unsigned)'0' < devices.size()) {
							index = (c - '0');
						}
					}
				}

				if(index < devices.size()) {
					if(!authorizeDevice(devices[index], hueConfig)) {
						fprintf(stderr, "Could not authorize device\n");
					}
				}
			} else if(listHubs) {
				if(jsonOutput) {
					printf("[");
					for(unsigned int i = 0; i < devices.size(); i++) {
						printf("%s", devices[i]->toJson().c_str());

						if(i + 1 < devices.size()) {
							printf(",");
						}
					}
					printf("]\n");
				} else {
					for(unsigned int i = 0; i < devices.size(); i++) {
						printf("%s\n", devices[i]->toString().c_str());
					}
				}
			}

			for(unsigned int i = 0; i < devices.size(); i++) {
				delete devices[i];
			}

			devices.clear();
		}
	}

	return 0;
} 
