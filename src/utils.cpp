#include <set>
#include <iostream>
#include "utils.h"

void commaListToSet(const std::string& str, std::set<std::string>& v) {
	size_t bpos = 0, epos = 0;
	do {
		epos = str.find(',', bpos);
		if(epos == std::string::npos && bpos < str.length()) {
			epos = str.length();
		}

		std::string item = str.substr(bpos, epos-bpos);
		if(item.length() > 0) {
			v.insert(item);
		}

		bpos = epos + 1;
	} while(epos != std::string::npos && bpos < str.length());
}

void commaListToVector(const std::string& str, std::vector<std::string>& v) {
	size_t bpos = 0, epos = 0;
	do {
		epos = str.find(',', bpos);
		if(epos == std::string::npos && bpos < str.length()) {
			epos = str.length();
		}

		std::string item = str.substr(bpos, epos-bpos);
		if(item.length() > 0) {
			v.push_back(item);
		}

		bpos = epos + 1;
	} while(epos != std::string::npos && bpos < str.length());
}
