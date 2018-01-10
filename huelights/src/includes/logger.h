#include <fstream>

enum LOGGER_LEVEL {
	LOGGER_LEVEL_DEBUG = 0,
	LOGGER_LEVEL_INFO,
	LOGGER_LEVEL_WARNING,
	LOGGER_LEVEL_ERROR,
};

class Logger {
private:
	static bool sEnabled;
	static std::ofstream sStream;

public:
	static void init();
	static void enable() {
		sEnabled = true;
	}

	static std::ostream& debug() {
		return log(LOGGER_LEVEL_DEBUG);
	}

	static std::ostream& info() {
		return log(LOGGER_LEVEL_INFO);
	}

	static std::ostream& warning() {
		return log(LOGGER_LEVEL_WARNING);
	}

	static std::ostream& error() {
		return log(LOGGER_LEVEL_ERROR);
	}

	static std::ostream& log(LOGGER_LEVEL level);
};
