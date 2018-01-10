#include <iostream>
#include <ctime>

#include "logger.h"

bool Logger::sEnabled = false;
std::ofstream Logger::sStream;

std::ostream &Logger::log(LOGGER_LEVEL level) {
	if(!sEnabled) {
		if(level <= LOGGER_LEVEL_INFO) {
			return std::cout;
		}

		return std::cerr;
	}

	if(!sStream.is_open()) {
		sStream.open("/var/log/huelights", std::ofstream::out | std::ofstream::app);
		sStream << std::unitbuf;
	}

	time_t rawtime;
	struct tm * timeinfo;
	char buffer[128];

	time (&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

	switch(level) {
		case LOGGER_LEVEL_DEBUG:
			sStream << "[Debug]";
			break;
		case LOGGER_LEVEL_WARNING:
			sStream << "[Warning]";
			break;
		case LOGGER_LEVEL_ERROR:
			sStream << "[Error]";
			break;
		default:
			sStream << "[Info]";
	}

	sStream << " " << std::string(buffer) << ": ";
	return sStream;
}