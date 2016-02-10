#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <unistd.h>

#include "hue/hue.h"

enum ArgCommand {
	ArgCommandNone = 0,
	ArgCommandDaemon,
	ArgCommandAuthorize,
	ArgCommandLight,
	ArgCommandList,
};

enum ArgListType {
	ArgListTypeNormal = 0,
	ArgListTypeJson,
};

static void printHelp() {
	std::cerr << "huelights [options] <command> [parameters]\n"
	<< "Options" << "\n"
	<< "\t" << "--help" << "\n"
	<< "\t\t" << "Show this help text" << "\n"
	<< "\t" << "--config <file>" << "\n"
	<< "\t\t" << "Specify the config file (defaults to /etc/huelights)" << "\n"
	<< "\t" << "--hub <id>" << "\n"
	<< "\t\t" << "Specify which hub device you want to operate on" << "\n"
	<< "\t" << "--light <id>" << "\n"
	<< "\t\t" << "Specify which light you want to operate on" << "\n"
	<< "\n"

	<< "Light options" << "\n"
	<< "\t" << "--brightness <brightness>" << "\n"
	<< "\t\t" << "Set the brightness of a light" << "\n"
	<< "\t\t" << "in the range 0-254 (where 0 is off and 254 is the brightest)" << "\n"
	<< "\n"

	<< "List options" << "\n"
	<< "\t" << "--json" << "\n"
	<< "\t\t" << "Output the list commands to json" << "\n"
	<< "\n"

	<< "Commands" << "\n"

	<< "\t" << "daemon" << "\n"
	<< "\t\t" << "Run in daemon mode" << "\n"
	<< "\t\t" << "This will run all tasks in the config file" << "\n"

	<< "\t" << "authorize" << "\n"
	<< "\t\t" << "Authorize a new hub device" << "\n"
	<< "\t\t" << "If you do not specify the hub with --hub you will be prompted" << "\n"

	<< "\t" << "light <on/off/toggle>" << "\n"
	<< "\t\t" << "Change the state of a light" << "\n"

	<< "\t" << "list <type>" << "\n"
	<< "\t\t" << "<type> can be one of" << "\n"
	<< "\t\t" << "hubs" << "\n"
	<< "\t\t\t" << "List the available hubs on the network" << "\n"
	<< "\t\t" << "lights" << "\n"
	<< "\t\t\t" << "List the lights associated with a specific hub" << "\n"
	<< "\t\t\t" << "Requires --hub to be set" << "\n"
	<< "\t\t" << "tasks" << "\n"
	<< "\t\t\t" << "List all configured tasks" << "\n"
	<< "\t\t\t" << "You can use --hub to specify the device, and --light for the light" << "\n";
}

static bool runDaemon(HueConfig& config, const std::vector<std::string> &params, bool& showHelp) {
	if(params.size() > 0) {
		showHelp = true;
		return false;
	}

	std::vector<HubDevice* > devices;

	bool error = false;
	while(!error) {
		// Calculate when the next minute starts
		time_t start = time(NULL);

		time_t end = start;
		struct tm* endTime = std::localtime(&end);
		++endTime->tm_min;
		endTime->tm_sec = 0;
		end = mktime(endTime);

		// Sleep until that time
		uint64_t diff = difftime(end, start);
		sleep(diff);

		if(!Hue::getHubDevices(devices, config)) {
			continue;
		}

		for(std::vector<HubDevice*>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt) {
			for(std::vector<HueTask*>::const_iterator it = (*deviceIt)->tasks().begin(); it != (*deviceIt)->tasks().end(); ++it) {
				(*it)->execute(error);
			}
		}

		bool parseFailure = false;
		if(!config.parse(parseFailure)) {
			if(parseFailure) {
				std::cerr << "Failed to parse config file!\n";
			} else {
				std::cerr << "Failed to read config file!\n";
			}
		}
	}

	for(std::vector<HubDevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
		delete *it;
	}

	return true;
}

static bool runAuthorize(HueConfig& config, const std::string& hubID, const std::vector<std::string> &params, bool& showHelp) {
	if(params.size() > 0) {
		showHelp = true;
		return false;
	}

	HubDevice* device = NULL;
	if(hubID.length() > 0) {
		device = Hue::getHubDevice(hubID, config);
	}

	std::vector<HubDevice* > devices;
	if(device == NULL) {
		if(Hue::getHubDevices(devices, config)) {
			if(devices.size() > 1) {
				std::cout << "Choose which device you want to authorize (0-" << (devices.size() - 1) << "):\n";
				for(size_t i = 0; i < devices.size(); i++) {
					std::cout << "[" << i << "] " << devices[i]->name() << " (" << devices[i]->ip() << ")\n";
				}

				int index = -1;

				std::string line = "";
				while(index == -1) {
					std::getline(std::cin, line);

					char* end;
					index = strtol(line.c_str(), &end, 10);
					if(*end || index < 0 || index >= (int)devices.size()) {
						index = -1;
						std::cerr << "Please enter a valid number:\n";
					}
				}
				
				device = devices[index];
			}
		}
	}

	if(device == NULL) {
		return false;
	}

	std::cout << "Authorizing device " << device->name() << "\n";
	std::cout << "Press the link button on the hub before the progress bar fills...\n";

	std::cout << "[";
	for(size_t i = 0; i < 20; i++) {
		std::cout << " ";
	}
	std::cout << "]" << std::flush;

	bool auth = false, retry = true;
	size_t times = 0;
	while(retry && times < 20) {
		if((auth = device->authorize(retry))) {
			std::cout << "Device authorized!\n";
			break;
		}

		sleep(1);
		times++;

		std::cout << "\r[";
		for(size_t i = 0; i < 20; i++) {
			if(i <= times) {
				std::cout << "#";
			} else {
				std::cout << " ";
			}
		}
		std::cout << "]" << std::flush;
	}

	if(times == 20) {
		std::cout << "\n";
	}

	bool ret = false;
	if(times < 20) {
		ret = device->isAuthorized();
		if(ret) {
			device->config().write();
		}
	}

	for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
		delete *it;
	}

	return ret;
}

static bool runLight(HueConfig& config, const std::string& hubID, const std::string& lightID, HueLightState& lightState, const std::vector<std::string> &params, bool& showHelp) {
	if(params.size() != 1 || hubID.length() == 0 || lightID.length() == 0) {
		showHelp = true;
		return false;
	}

	showHelp = false;

	HubDevice* device = Hue::getHubDevice(hubID, config);
	if(device == NULL) {
		std::cerr << "Error: Failed to find device " << hubID << "\n";
		return false;
	}

	if(!device->isAuthorized()) {
		std::cerr << "Error: Device " << hubID << " is not authorized!\n";
		return false;
	}

	HueLight* light = device->light(lightID);
	if(light == NULL) {
		std::cerr << "Error: Failed to find light " << lightID << "\n";

		delete device;
		return false;
	}

	lightState.setOn(light->state()->on());
	
	std::string state = params[0];
	if(state == "on") {
		lightState.setOn();
	} else if(state == "off") {
		lightState.setOn(false);
	} else if(state == "toggle") {
		lightState.toggle();
	} else {
		std::cerr << "Error: Unknown light state " << state << "\n";

		delete light;
		delete device;
		return false;
	}

	lightState.copyTo(light->newState());

	bool b = light->write(*device);

	delete device;

	return b;
}

static bool runList(HueConfig& config, const std::string& hubID, const std::string& lightID, ArgListType listType, const std::vector<std::string> &params, bool& showHelp) {
	showHelp = false;

	if(params.size() != 1) {
		showHelp = true;
		return false;
	}

	std::string type = params[0];
	if(type == "hubs") {
		if(hubID.size() != 0 || lightID.size() != 0) {
			showHelp = true;
			return false;
		}

		std::vector<HubDevice* > devices;
		if(!Hue::getHubDevices(devices, config)) {
			std::cerr << "Error: Failed to get devices\n";
			return false;
		}

		switch(listType) {
			case ArgListTypeNormal: {
				for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
					std::cout << (*it)->toString() << "\n\n";
				}

				break;
			}
			case ArgListTypeJson: {
				json_object* arrObj = json_object_new_array();
				for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
					json_object_array_add(arrObj, (*it)->toJson());
				}

				std::cout << json_object_to_json_string_ext(arrObj, JSON_C_TO_STRING_PLAIN);
				json_object_put(arrObj);

				break;
			}
		}

		for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
			delete *it;
		}
	} else if(type == "lights") {
		if(hubID.size() == 0 || lightID.size() != 0) {
			showHelp = true;
			return false;
		}

		HubDevice* device = Hue::getHubDevice(hubID, config);
		if(device == NULL) {
			std::cerr << "Error: Failed to find device " << hubID << "\n";
			return false;
		}

		if(!device->isAuthorized()) {
			std::cerr << "Error: Device " << hubID << " is not authorized!\n";
			return false;
		}

		switch(listType) {
			case ArgListTypeNormal: {
				for(std::vector<HueLight*>::const_iterator it = device->lights().begin(); it != device->lights().end(); ++it) {
					std::cout << (*it)->toString() << "\n\n";
				}

				break;
			}
			case ArgListTypeJson: {
				json_object* arrObj = json_object_new_array();
				for(std::vector<HueLight*>::const_iterator it = device->lights().begin(); it != device->lights().end(); ++it) {
					json_object_array_add(arrObj, (*it)->toJson());
				}

				std::cout << json_object_to_json_string_ext(arrObj, JSON_C_TO_STRING_PLAIN);
				json_object_put(arrObj);

				break;
			}
		}
	} else if(type == "tasks") {
		std::vector<HubDevice* > devices;
		if(hubID.size() > 0) {
			HubDevice* device = Hue::getHubDevice(hubID, config);
			if(device == NULL) {
				std::cerr << "Error: Failed to get device " << hubID << "\n";
				return false;
			}

			devices.push_back(device);
		} else {
			if(!Hue::getHubDevices(devices, config)) {
				std::cerr << "Error: Failed to get devices\n";
				return false;
			}
		}

		time_t now = time(NULL);

		switch(listType) {
			case ArgListTypeNormal: {
				for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
					for(std::vector<HueTask*>::const_iterator taskIt = (*it)->tasks().begin(); taskIt != (*it)->tasks().end(); ++taskIt) {
						(*taskIt)->updateTrigger(now);
						std::cout << (*taskIt)->toString() << "\n\n";
					}
				}

				break;
			}
			case ArgListTypeJson: {
				json_object* arrObj = json_object_new_array();
				for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
					for(std::vector<HueTask*>::const_iterator taskIt = (*it)->tasks().begin(); taskIt != (*it)->tasks().end(); ++taskIt) {
						(*taskIt)->updateTrigger(now);
						json_object_array_add(arrObj, (*taskIt)->toJson());
					}
				}

				std::cout << json_object_to_json_string_ext(arrObj, JSON_C_TO_STRING_PLAIN);
				json_object_put(arrObj);

				break;
			}
		}

		for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
			delete *it;
		}
	}

	return true;
}

int main(int argc, char** argv) {
	std::string config = "/etc/huelights", hubID = "", lightID = "";
	ArgListType listType = ArgListTypeNormal;
	HueLightState lightState;

	ArgCommand argCommand = ArgCommandNone;
	std::vector<std::string> argParams;

	srand(time(NULL));

	if(argc == 1) {
		printHelp();
		return -1;
	}

	for(int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if(argCommand != ArgCommandNone) {
			argParams.push_back(arg);
			continue;
		}

		if(arg == "--help") {
			printHelp();
			return -1;
		} else if(arg == "--config") {
			if(i + 1 >= argc) {
				printHelp();
				return -1;
			}

			config = std::string(argv[i + 1]);
			i++;
		} else if(arg == "--hub") {
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
		} else if(arg == "--brightness") {
			if(i + 1 >= argc) {
				printHelp();
				return -1;
			}

			lightState.setBrightness(atoi(argv[i + 1]));
			i++;
		} else if(arg == "--json") {
			listType = ArgListTypeJson;
		} else if(arg == "daemon") {
			argCommand = ArgCommandDaemon;
		} else if(arg == "authorize") {
			argCommand = ArgCommandAuthorize;
		} else if(arg == "light") {
			argCommand = ArgCommandLight;
		} else if(arg == "list") {
			argCommand = ArgCommandList;
		} else {
			std::cerr << "Error: Unknown option " << argv[i] << "\n";
			printHelp();
			return -1;
		}
	}

	HueConfig hueConfig(config);

	bool parseFailure = false;
	if(!hueConfig.parse(parseFailure)) {
		if(parseFailure) {
			std::cerr << "Warning: Failed to parse config file " << config << ", continuing without using a config file...\n";
			return -1;
		}
	}

	bool showHelp = false;
	switch(argCommand) {
		case ArgCommandDaemon: {
			if(!runDaemon(hueConfig, argParams, showHelp)) {
				if(showHelp) {
					printHelp();
					return -1;
				}

				std::cerr << "Error: Daemon command failed\n";
			}

			break;
		}
		case ArgCommandAuthorize: {
			if(!runAuthorize(hueConfig, hubID, argParams, showHelp)) {
				if(showHelp) {
					printHelp();
					return -1;
				}

				std::cerr << "Error: Authorization command failed\n";
			}

			break;
		}
		case ArgCommandLight: {
			if(!runLight(hueConfig, hubID, lightID, lightState, argParams, showHelp)) {
				if(showHelp) {
					printHelp();
					return -1;
				}

				std::cerr << "Error: Light command failed\n";
			}

			break;
		}
		case ArgCommandList: {
			if(!runList(hueConfig, hubID, lightID, listType, argParams, showHelp)) {
				if(showHelp) {
					printHelp();
					return -1;
				}

				std::cerr << "Error: List command failed\n";
			}

			break;
		}
		default:
			break;
	}

	return 0;
} 
