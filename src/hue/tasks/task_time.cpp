#include <sstream>
#include <cstring>
#include <unistd.h>
#include "hue/tasks/task_time.h"
#include "utils.h"
#include "sunposition.h"

std::map<std::string, HueTaskTime::Method> HueTaskTime::sSupportedMethods;

HueTaskTime::HueTaskTime(const HueConfigSection &taskConfig, const HueConfigSection &stateConfig, const HueConfigSection &triggerConfig, const HubDevice& device)
	: HueTask(taskConfig, stateConfig, device),
	mTaskMethod(HueTaskTime::MethodNone),
	mPosition(std::make_pair<double, double>(0.0f, 0.0f)),
	mTimeSun(SunNone)
{
	if(sSupportedMethods.size() == 0) {
		sSupportedMethods.insert(std::pair<std::string, HueTaskTime::Method>("fixed", HueTaskTime::MethodFixed));
		sSupportedMethods.insert(std::pair<std::string, HueTaskTime::Method>("recurring", HueTaskTime::MethodRecurring));
	}

	std::string m = triggerConfig.value("method");
	if(sSupportedMethods.count(m) != 1) {
		HueTask::mValid = false;
		return;
	}

	mTaskMethod = sSupportedMethods.at(m);

	memset(&mTime, 0, sizeof(struct tm));

	switch(mTaskMethod) {
		case HueTaskTime::MethodFixed: {
			if (!strptime(triggerConfig.value("time").c_str(), "%Y-%m-%d %H:%M", &mTime)) {
				HueTask::mValid = false;
				return;
		    }

			break;
		}
		case HueTaskTime::MethodRecurring: {
			std::string timeStr = triggerConfig.value("time");
			if(timeStr == "sunrise") {
				mTimeSun = SunRise;
			} else if(timeStr == "sunset") {
				mTimeSun = SunSet;
			} else if (!strptime(timeStr.c_str(), "%H:%M", &mTime)) {
				HueTask::mValid = false;
				return;
		    }

		    if(triggerConfig.hasKey("position")) {
				std::vector<std::string> position;
				commaListToVector(triggerConfig.value("position"), position);

				mPosition.first = atof(position.at(0).c_str());
				mPosition.second = atof(position.at(1).c_str());
		    }

		    if(triggerConfig.hasKey("days")) {
				std::set<std::string> repeat;
				commaListToSet(triggerConfig.value("days"), repeat);
		    	
				if(repeat.find("all") != repeat.end()) {
					for(uint32_t i = 0; i < 7; i++) {
						mRepeatDays.insert(i);
					}
				} else {
					static std::string days[] = {"sun", "mon", "tue", "wed", "thu", "fri", "sat"};

					for(std::set<std::string>::iterator it = repeat.begin(); it != repeat.end(); ++it) {
						for(uint32_t i = 0; i < 7; i++) {
							if(*it == days[i]) {
								mRepeatDays.insert(i);
							}
						}
					}
				}
		    } else {
		    	for(uint32_t i = 0; i < 7; i++) {
	    			mRepeatDays.insert(i);
	    		}
		    }
		}
		default: {
			break;
		}
	}
}

bool HueTaskTime::execute(bool& fatalError) {
	time_t now;
    time(&now);

    int64_t diff = -1;
	if(mTime.tm_year > 0) {
		diff = difftime(now, mktime(&mTime));
	}

    switch(mTaskMethod) {
    	case MethodFixed: {
    		if(diff >= 0 && diff < 60) {
		    	std::cout << "TRIGGER!\n";
				trigger();
		    }

    		break;
    	}
    	case MethodRecurring: {
    		// We need to re-calculate when to trigger the next alarm.
    		if(mTime.tm_year == 0 || diff >= 0) {
				updateTrigger(now, &diff);
		    }
    		
    		if(diff >= 0 && diff < 60) {
		    	std::cout << "TRIGGER!\n";
				trigger();
		    }

    		break;
    	}
    	default: {
    		break;
    	}
    }
 
	return true;
}

void HueTaskTime::updateTrigger(time_t now, int64_t* diff) {
	if(mTaskMethod != MethodRecurring) {
		return;
	}

	struct tm* nowTm = localtime(&now);

	mTime.tm_year = nowTm->tm_year;
	mTime.tm_mon = nowTm->tm_mon;
	mTime.tm_mday = nowTm->tm_mday;
	mTime.tm_wday = nowTm->tm_wday;
	mTime.tm_sec = 0;
	mTime.tm_yday = nowTm->tm_yday;
	mTime.tm_isdst = nowTm->tm_isdst;

	if(mRepeatDays.size() > 0) {
		// Two weeks ahead should be enough to find a valid date (probably not necessary, but it doesn't matter).
		for(int i = 0; i < 14; i++) {
			if(mRepeatDays.find(mTime.tm_wday) != mRepeatDays.end()) {
				if(mTimeSun != SunNone) {
					std::pair<time_t, time_t> sunPosition;
					SunPosition::getTimes(sunPosition, mktime(&mTime), mPosition.first, mPosition.second);

					struct tm* sunTime;
					if(mTimeSun == SunRise) {
						sunTime = localtime(&sunPosition.first);
					} else {
						sunTime = localtime(&sunPosition.second);
					}

					mTime.tm_hour = sunTime->tm_hour;
					mTime.tm_min = sunTime->tm_min;
				}

				// If we have just started the program, diff will be -1, and won't trigger, even if the next minute is a match.
				// Work around this by setting diff to diff2 when diff2 is 0 and diff is unset.
				int64_t diff2 = difftime(now, mktime(&mTime));
				if(diff2 < 0 || (diff != NULL && diff2 == 0 && *diff < 0)) {
					if(diff != NULL && *diff < 0) {
						*diff = diff2;
					}

					break;
				}

				if(mTimeSun != SunNone) {
					mTime.tm_hour = mTime.tm_min = 0;
				}
			}

			mTime.tm_mday++;
			mTime.tm_wday++;
			if(mTime.tm_wday > 6) {
				mTime.tm_wday = 0;
			}
		}
	}
}

void HueTaskTime::toJsonInt(json_object* obj) const {
	char buf[80];
	strftime(buf, 80, "%Y-%m-%d %H:%M", &mTime);

	std::string timeStr = buf;
	if(mTimeSun == SunRise) {
		timeStr += " (sunrise)";
	} else if(mTimeSun == SunSet) {
		timeStr += " (sunset)";
	}

	json_object* triggerObj = json_object_new_object();

	for(std::map<std::string, HueTaskTime::Method>::iterator it = sSupportedMethods.begin(); it != sSupportedMethods.end(); ++it) {
		if(it->second == mTaskMethod) {
			json_object_object_add(triggerObj, "method", json_object_new_string(it->first.c_str()));
			break;
		}
	}

	json_object_object_add(triggerObj, "time", json_object_new_string(timeStr.c_str()));

	json_object_object_add(obj, "trigger", triggerObj);
}

void HueTaskTime::toStringInt(std::ostringstream& s) const {
	char buf[80];
	strftime(buf, 80, "%Y-%m-%d %H:%M", &mTime);

	s << "\n\n" << "[Task " << id() << " Trigger]";
	s << "\n" << "method=";
	for(std::map<std::string, HueTaskTime::Method>::iterator it = sSupportedMethods.begin(); it != sSupportedMethods.end(); ++it) {
		if(it->second == mTaskMethod) {
			s << it->first;
			break;
		}
	}
	s << "\n" << "time=" << buf;

	if(mTimeSun == SunRise) {
		s << " (sunrise)";
	} else if(mTimeSun == SunSet) {
		s << " (sunset)";
	}
}
