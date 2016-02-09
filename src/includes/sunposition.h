#ifndef INCLUDES_SUNPOSITION_H
#define INCLUDES_SUNPOSITION_H

#include <iostream>
#include <ctime>

class SunPosition {
public:
	static bool getTimes(std::pair<time_t, time_t>& times, time_t date, double lat, double lng);

private:
	SunPosition();
}; 

#endif //INCLUDES_SUNPOSITION_H
