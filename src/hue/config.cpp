#include <cstdio>
#include <sstream>
#include "hue/config.h"

HueConfig::HueConfig(const std::string &path)
	: mPath(path)
{

}

HueConfig::~HueConfig() {
	for(size_t i = 0; i < mSections.size(); i++) {
		delete mSections[i];
	}

	mSections.clear();
}

HueConfigSection* HueConfig::getSection(const std::string& name, std::string key, std::string value) const {
	for(size_t i = 0; i < mSections.size(); i++) {
		if(mSections[i]->name() != name) {
			continue;
		}

		if(key.size() == 0 || mSections[i]->hasKey(key, value)) {
			return mSections[i];
		}
	}

	return NULL;
}

HueConfigSection* HueConfig::newSection(const std::string& name) {
	HueConfigSection* section = new HueConfigSection(name);
	mSections.push_back(section);
	return section;
}

bool HueConfig::parse() {
	bool ret = false;

	if(mSections.size() > 0) {
		for(size_t i = 0; i < mSections.size(); i++) {
			delete mSections[i];
		}

		mSections.clear();
	}

	FILE* file = fopen(mPath.c_str(), "r");
	if(!file) {
		return false;
	}

	char buf[256];
	std::string section = "";
	while(!feof(file)) {
		char* p = fgets(buf, 256, file);
		if(feof(file) || buf[0] == '[') {
			if(feof(file) && p != NULL && buf[0] != '[') {
				section += std::string(buf);
			}

			// Figure out what we need to do
			if(section.size() > 0) {
				HueConfigSection* configSection = new HueConfigSection();
				if(!configSection->parse(section)) {
					ret = false;
					break;
				}

				mSections.push_back(configSection);
				section = "";
			}

			if(!feof(file)) {
				section += std::string(buf);
			} else {
				if(p != NULL && buf[0] == '[') {
					section += std::string(buf);

					HueConfigSection* configSection = new HueConfigSection();
					if(!configSection->parse(section)) {
						ret = false;
						break;
					}

					mSections.push_back(configSection);
				}


				ret = true;
			}
		} else {
			section += std::string(buf);
		}
	}

	fclose(file);

	if(!ret && mSections.size() > 0) {
		for(size_t i = 0; i < mSections.size(); i++) {
			delete mSections[i];
		}

		mSections.clear();
	}

	return ret;
}

bool HueConfig::write() {
	FILE* file = fopen(mPath.c_str(), "w+");
	if(!file) {
		return false;
	}

	for(size_t i = 0; i < mSections.size(); i++) {
		HueConfigSection* section = mSections[i];
		fprintf(file, "[%s]\n", section->name().c_str());

		std::vector<std::pair<std::string, std::string> >::const_iterator it;
		for(it = section->begin(); it != section->end(); ++it) {
			fprintf(file, "%s=%s\n", (*it).first.c_str(), (*it).second.c_str());
		}
	}

	fflush(file);
	fclose(file);

	return true;
}

HueConfigSection::HueConfigSection(std::string name)
	: mName(name)
{

}

bool HueConfigSection::hasKey(const std::string &key, std::string value) const {
	for(size_t i = 0; i < mValues.size(); i++) {
		if(mValues[i].first == key && (value.size() == 0 || mValues[i].second == value)) {
			return true;
		}
	}

	return false;
}

std::string HueConfigSection::value(const std::string &key) const {
	for(size_t i = 0; i < mValues.size(); i++) {
		if(mValues[i].first == key) {
			return mValues[i].second;
		}
	}

	return "";
}

void HueConfigSection::setValue(const std::string& key, const std::string& value) {
	for(size_t i = 0; i < mValues.size(); i++) {
		if(mValues[i].first == key) {
			mValues[i].second = value;
			return;
		}
	}

	mValues.push_back(std::make_pair(key, value));
}

bool HueConfigSection::parse(const std::string& content) {
	std::istringstream iss(content);

	std::string line;
	if(std::getline(iss, line)) {
		size_t pos = line.find(']');
		if(pos == std::string::npos) {
			return false;
		}

		mName = line.substr(1, pos - 1);
		if(mName.size() == 0) {
			return false;
		}
	}

	while(std::getline(iss, line)) {
		size_t pos = line.find('=');
		if(line.size() > 0 && pos == std::string::npos) {
			return false;
		} else if(pos == std::string::npos) {
			continue;
		}

		if(pos == 0) {
			return false;
		}

		std::string key = line.substr(0, pos);
		std::string value = line.substr(pos + 1);
		mValues.push_back(std::make_pair(key, value));
	}

	return true;
}
