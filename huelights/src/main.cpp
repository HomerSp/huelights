#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <unistd.h>

#include "logger.h"
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

enum ArgTypes {
	ArgTypeConfig,
	ArgTypeHub,
	ArgTypeLight,
	ArgTypeLightState,
	ArgTypeLightBrightness,
	ArgTypeListDisplay,
	ArgTypeListType,
};

static void printHelp() {
	Logger::error() << "huelights [options]\n"
	<< "Options" << "\n"
	<< "\t" << "-h, --help" << "\n"
	<< "\t\t" << "Show this help text" << "\n"
	<< "\t" << "-c, --config <file>" << "\n"
	<< "\t\t" << "Specify the config file (defaults to /etc/huelights)" << "\n"
	<< "\t" << "--hub <id>" << "\n"
	<< "\t\t" << "Specify the hub device" << "\n"
	<< "\n"
	<< "\t" << "--light <id>" << "\n"
	<< "\t\t" << "Specify which light you want to operate on" << "\n"
	<< "\t" << "--state <state>" << "\n"
	<< "\t\t" << "Set the state of a light, must be either on, off or toggle" << "\n"
	<< "\t" << "--brightness <brightness>" << "\n"
	<< "\t\t" << "Set the brightness of a light" << "\n"
	<< "\t\t" << "in the range 0-254 (where 0 is off and 254 is the brightest)" << "\n"
	<< "\n"

	<< "\t" << "-d, --daemon" << "\n"
	<< "\t\t" << "Run in daemon mode" << "\n"
	<< "\t\t" << "This will run all tasks in the config file" << "\n"

	<< "\t" << "-a, --authorize" << "\n"
	<< "\t\t" << "Authorize a new hub device" << "\n"
	<< "\t\t" << "If you do not specify the hub with --hub you will be prompted" << "\n"

	<< "\t" << "-l, --list <type>" << "\n"
	<< "\t\t" << "<type> can be one of" << "\n"
	<< "\t\t" << "hubs" << "\n"
	<< "\t\t\t" << "List the available hubs on the network" << "\n"
	<< "\t\t" << "lights" << "\n"
	<< "\t\t\t" << "List the lights associated with a specific hub" << "\n"
	<< "\t\t\t" << "or list all lights on all found hubs" << "\n"
	<< "\t\t" << "tasks" << "\n"
	<< "\t\t\t" << "List all configured tasks" << "\n"
	<< "\t\t\t" << "You can use --hub to specify the device, and/or --light for the light" << "\n"
	<< "\t" << "--json" << "\n"
	<< "\t\t" << "Output the list commands in json format" << "\n"
	<< "\n";
}

static bool runDaemon(HueConfig& config, const std::map<ArgTypes, std::string> &params, bool& showHelp) {
	std::vector<HubDevice* > devices;

	Logger::enable();
	Logger::info() << "\n";
	Logger::info() << "Starting daemon\n";

	std::map<std::string, int> failedTasks;
	std::vector<std::string> permanentFailedTasks;
	time_t lastTime = time(NULL);
	bool running = true;
	while(running) {
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

		bool parseFailure = false;
		if(!config.parse(parseFailure)) {
			if(parseFailure) {
				Logger::error() << "Failed to parse config file!\n";
			} else {
				Logger::error() << "Failed to read config file!\n";
			}

			lastTime = time(NULL);
			continue;
		}

		if(!Hue::getHubDevices(devices, config)) {
			lastTime = time(NULL);
			continue;
		}

		for(std::vector<HubDevice*>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt) {
			for(std::vector<HueTask*>::const_iterator it = (*deviceIt)->tasks().begin(); it != (*deviceIt)->tasks().end(); ++it) {
				// We're going back in time, call the troops (reset the task)!
				if(start < lastTime || start - lastTime > 60*2) {
					(*it)->reset();
				}

				std::string id = (*it)->id();
				bool result = true;
				if(std::find(permanentFailedTasks.begin(), permanentFailedTasks.end(), id) == permanentFailedTasks.end() && failedTasks.find(id) != failedTasks.end()) {
					result = (*it)->executeNow();
					if(result) {
						failedTasks.erase(id);
					} else if(failedTasks.find(id)->second >= 4) {
						permanentFailedTasks.push_back(id);
					}
				}

				bool done = true;
				bool error = false;
				if(result) {
					done = (*it)->execute(error);
					if(done && !error) {
						permanentFailedTasks.erase(std::find(permanentFailedTasks.begin(), permanentFailedTasks.end(), id));
						failedTasks.erase(id);
					}
				}

				if(!result || (done && error)) {
					int retryCount = 1;
					if(failedTasks.find((*it)->id()) != failedTasks.end()) {
						retryCount += failedTasks.find((*it)->id())->second;
					}

					failedTasks[(*it)->id()] = retryCount;
				}
			}
		}

		lastTime = time(NULL);
	}

	for(std::vector<HubDevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
		delete *it;
	}

	return true;
}

static bool runAuthorize(HueConfig& config, const std::map<ArgTypes, std::string> &params, bool& showHelp) {
	HubDevice* device = NULL;
	if(params.count(ArgTypeHub) == 1) {
		device = Hue::getHubDevice(params.at(ArgTypeHub), config);
	} else {
		std::vector<HubDevice* > devices;
		if(Hue::getHubDevices(devices, config)) {
			if(devices.size() >= 1) {
				Logger::info() << "Choose which device you want to authorize (0-" << (devices.size() - 1) << "):\n";
				for(size_t i = 0; i < devices.size(); i++) {
					Logger::info() << "[" << i << "] " << devices[i]->name() << " (" << devices[i]->ip() << ")\n";
				}

				int index = -1;

				std::string line = "";
				while(index == -1) {
					std::getline(std::cin, line);

					char* end;
					index = strtol(line.c_str(), &end, 10);
					if(*end || index < 0 || index >= (int)devices.size()) {
						index = -1;
						Logger::error() << "Please enter a valid number:\n";
					}
				}
				
				device = devices[index];
			}
		}


		for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
			if(device != NULL && *it == device) {
				continue;
			}

			delete *it;
		}
	}

	if(device == NULL) {
		return false;
	}

	Logger::info() << "Authorizing device " << device->name() << "\n";
	Logger::info() << "Press the link button on the hub before the progress bar fills...\n";

	Logger::info() << "[";
	for(size_t i = 0; i < 20; i++) {
		Logger::info() << " ";
	}
	Logger::info() << "]" << std::flush;

	bool auth = false, retry = true;
	size_t times = 0;
	while(retry && times < 20) {
		if((auth = device->authorize(retry))) {
			Logger::info() << "\rDevice authorized!\n";
			break;
		}

		sleep(1);
		times++;

		Logger::info() << "\r[";
		for(size_t i = 0; i < 20; i++) {
			if(i <= times) {
				Logger::info() << "#";
			} else {
				Logger::info() << " ";
			}
		}
		Logger::info() << "]" << std::flush;
	}

	if(times == 20) {
		Logger::info() << "\n";
	}

	bool ret = false;
	if(times < 20) {
		ret = device->isAuthorized();
		if(ret) {
			device->config().write();
		}
	}

	return ret;
}

static bool runLight(HueConfig& config, const std::map<ArgTypes, std::string> &params, bool& showHelp) {
	if(params.count(ArgTypeLight) == 0) {
		showHelp = true;
		return false;
	}

	HubDevice* device = NULL;
	if(params.count(ArgTypeHub) != 0) {
		device = Hue::getHubDevice(params.at(ArgTypeHub), config);
		if(device == NULL) {
			Logger::error() << "Error: Failed to find hub " << params.at(ArgTypeHub) << "\n";
			return false;
		}
	} else {
		// Try to find out which hub this light is attached to.
		std::vector<HubDevice* > devices;
		if(Hue::getHubDevices(devices, config)) {
			for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
				if((*it)->light(params.at(ArgTypeLight)) != NULL) {
					device = (*it);
				} else {
					delete *it;
				}
			}
		}

		if(device == NULL) {
			Logger::error() << "Error: Failed to find the device for light " << params.at(ArgTypeLight) << "\n";
			return false;
		}
	}

	if(!device->isAuthorized()) {
		Logger::error() << "Error: Device " << device->name() << " is not authorized!\n";
		return false;
	}

	HueLight* light = device->light(params.at(ArgTypeLight));
	if(light == NULL) {
		Logger::error() << "Error: Failed to find light " << params.at(ArgTypeLight) << "\n";

		delete device;
		return false;
	}

	HueLightState lightState(light->state());
	lightState.reset();

	if(params.count(ArgTypeLightState) != 0) {
		std::string state = params.at(ArgTypeLightState);
		if(state == "on") {
			lightState.setOn();
		} else if(state == "off") {
			lightState.setOn(false);
		} else if(state == "toggle") {
			lightState.toggle();
		} else {
			Logger::error() << "Error: Unknown light state " << state << "\n";
			showHelp = true;

			delete light;
			delete device;
			return false;
		}
	}
	if(params.count(ArgTypeLightBrightness) != 0) {
		lightState.setBrightness(atoi(params.at(ArgTypeLightBrightness).c_str()));
	}

	lightState.copyTo(light->newState());

	bool b = light->write(*device);

	delete device;

	return b;
}

static bool runList(HueConfig& config, const std::map<ArgTypes, std::string> &params, bool& showHelp) {
	if(params.count(ArgTypeListType) == 0) {
		showHelp = true;
		return false;
	}

	ArgListType listType = ArgListTypeNormal;
	if(params.count(ArgTypeListDisplay) != 0) {
		if(params.at(ArgTypeListDisplay) == "json") {
			listType = ArgListTypeJson;
		}
	}

	std::string type = params.at(ArgTypeListType);
	if(type == "hubs") {
		std::vector<HubDevice* > devices;
		if(!Hue::getHubDevices(devices, config)) {
			Logger::error() << "Error: Failed to get devices\n";
			return false;
		}

		switch(listType) {
			case ArgListTypeNormal: {
				for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
					Logger::info() << (*it)->toString() << "\n\n";
				}

				break;
			}
			case ArgListTypeJson: {
				json_object* arrObj = json_object_new_array();
				for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
					json_object_array_add(arrObj, (*it)->toJson());
				}

				Logger::info() << json_object_to_json_string_ext(arrObj, JSON_C_TO_STRING_PLAIN);
				json_object_put(arrObj);

				break;
			}
		}

		for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
			delete *it;
		}
	} else if(type == "lights") {
		std::vector<HubDevice* > devices;
		if(params.count(ArgTypeHub) != 0) {
			HubDevice* device = Hue::getHubDevice(params.at(ArgTypeHub), config);
			if(device == NULL) {
				Logger::error() << "Error: Failed to find device " << params.at(ArgTypeHub) << "\n";
				return false;
			}

			devices.push_back(device);
		} else {
			if(!Hue::getHubDevices(devices, config)) {
				Logger::error() << "Error: Failed to get devices\n";
				return false;
			}
		}

		for(std::vector<HubDevice*>::const_iterator devIt = devices.begin(); devIt != devices.end(); ++devIt) {
			if(!(*devIt)->isAuthorized()) {
				continue;
			}

			switch(listType) {
				case ArgListTypeNormal: {
					for(std::vector<HueLight*>::const_iterator it = (*devIt)->lights().begin(); it != (*devIt)->lights().end(); ++it) {
						Logger::info() << (*it)->toString() << "\n\n";
					}

					break;
				}
				case ArgListTypeJson: {
					json_object* arrObj = json_object_new_array();
					for(std::vector<HueLight*>::const_iterator it = (*devIt)->lights().begin(); it != (*devIt)->lights().end(); ++it) {
						json_object_array_add(arrObj, (*it)->toJson());
					}

					Logger::info() << json_object_to_json_string_ext(arrObj, JSON_C_TO_STRING_PLAIN);
					json_object_put(arrObj);

					break;
				}
			}
		}

		for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
			delete *it;
		}
	} else if(type == "tasks") {
		std::vector<HubDevice* > devices;
		if(params.count(ArgTypeHub) != 0) {
			HubDevice* device = Hue::getHubDevice(params.at(ArgTypeHub), config);
			if(device == NULL) {
				Logger::error() << "Error: Failed to get device " << params.at(ArgTypeHub) << "\n";
				return false;
			}

			devices.push_back(device);
		} else {
			if(!Hue::getHubDevices(devices, config)) {
				Logger::error() << "Error: Failed to get devices\n";
				return false;
			}
		}

		time_t now = time(NULL);

		switch(listType) {
			case ArgListTypeNormal: {
				for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
					for(std::vector<HueTask*>::const_iterator taskIt = (*it)->tasks().begin(); taskIt != (*it)->tasks().end(); ++taskIt) {
						(*taskIt)->updateTrigger(now);
						Logger::info() << (*taskIt)->toString() << "\n\n";
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

				Logger::info() << json_object_to_json_string_ext(arrObj, JSON_C_TO_STRING_PLAIN);
				json_object_put(arrObj);

				break;
			}
		}

		for(std::vector<HubDevice*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
			delete *it;
		}
	} else {
		showHelp = true;
		return false;
	}

	return true;
}

int main(int argc, char** argv) {
	ArgCommand argCommand = ArgCommandNone;
	std::map<ArgTypes, std::string> argParams;

	srand(time(NULL));

	if(argc == 1) {
		printHelp();
		return -1;
	}

	for(int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if(arg == "--help") {
			printHelp();
			return -1;
		} else if(arg == "-c" || arg == "--config") {
			if(i + 1 >= argc) {
				printHelp();
				return -1;
			}

			argParams.insert(std::make_pair<ArgTypes, std::string>(ArgTypeConfig, std::string(argv[i + 1])));
			i++;
		} else if(arg == "--hub") {
			if(i + 1 >= argc) {
				printHelp();
				return -1;
			}

			argParams.insert(std::make_pair<ArgTypes, std::string>(ArgTypeHub, std::string(argv[i + 1])));
			i++;
		} else if(arg == "--light") {
			if(i + 1 >= argc) {
				printHelp();
				return -1;
			}

			argCommand = ArgCommandLight;
			argParams.insert(std::make_pair<ArgTypes, std::string>(ArgTypeLight, std::string(argv[i + 1])));
			i++;
		} else if(arg == "--brightness") {
			if(i + 1 >= argc) {
				printHelp();
				return -1;
			}

			char* p = NULL;
			int val =strtol(argv[i + 1], &p, 10);
			if(*p != 0 || val < 0 || val > 254) {
				Logger::warning() << argv[i + 1] << " is not a valid number (must be within 0-254)\n";
				printHelp();
				return -1;
			}

			argParams.insert(std::make_pair<ArgTypes, std::string>(ArgTypeLightBrightness, std::string(argv[i + 1])));
			i++;
		} else if(arg == "--state") {
			if(i + 1 >= argc) {
				printHelp();
				return -1;
			}

			argParams.insert(std::make_pair<ArgTypes, std::string>(ArgTypeLightState, std::string(argv[i + 1])));
			i++;
		} else if(arg == "--json") {
			argParams.insert(std::make_pair<ArgTypes, std::string>(ArgTypeListDisplay, "json"));
		} else if(arg == "-d" || arg == "--daemon") {
			argCommand = ArgCommandDaemon;
		} else if(arg == "-a" || arg == "--authorize") {
			argCommand = ArgCommandAuthorize;
		} else if(arg == "-l" || arg == "--list") {
			if(i + 1 >= argc) {
				printHelp();
				return -1;
			}

			argCommand = ArgCommandList;
			argParams.insert(std::make_pair<ArgTypes, std::string>(ArgTypeListType, std::string(argv[i + 1])));
			i++;
		} else {
			Logger::error() << "Error: Unknown option " << argv[i] << "\n";
			printHelp();
			return -1;
		}
	}

	std::string configPath = "/etc/huelights";
	if(argParams.count(ArgTypeConfig) == 1) {
		configPath = argParams.at(ArgTypeConfig);
	}

	HueConfig hueConfig(configPath);

	bool parseFailure = false;
	if(!hueConfig.parse(parseFailure)) {
		if(parseFailure) {
			Logger::error() << "Warning: Failed to parse config file " << configPath << ", exiting...\n";
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

				Logger::error() << "Error: Daemon command failed\n";
			}

			break;
		}
		case ArgCommandAuthorize: {
			if(!runAuthorize(hueConfig, argParams, showHelp)) {
				if(showHelp) {
					printHelp();
					return -1;
				}

				Logger::error() << "Error: Authorization command failed\n";
			}

			break;
		}
		case ArgCommandLight: {
			if(!runLight(hueConfig, argParams, showHelp)) {
				if(showHelp) {
					printHelp();
					return -1;
				}

				Logger::error() << "Error: Light command failed\n";
			}

			break;
		}
		case ArgCommandList: {
			if(!runList(hueConfig, argParams, showHelp)) {
				if(showHelp) {
					printHelp();
					return -1;
				}

				Logger::error() << "Error: List command failed\n";
			}

			break;
		}
		default:
			break;
	}

	return 0;
} 
