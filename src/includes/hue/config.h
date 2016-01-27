#ifndef INCLUDES_HUE_CONFIG_H
#define INCLUDES_HUE_CONFIG_H

#include <string>
#include <vector>

class HueConfigSection;

class HueConfig {
public:
	HueConfig(const std::string &path);
	~HueConfig();

	HueConfigSection* getSection(const std::string& name, std::string key = "", std::string value = "") const;
	HueConfigSection* newSection(const std::string& name);

	bool parse();
	bool write();

private:
	std::string mPath;
	std::vector<HueConfigSection* > mSections;

};

class HueConfigSection {
public:
	HueConfigSection(std::string name = "");

	const std::vector<std::pair<std::string, std::string> >::const_iterator begin() const {
		return mValues.begin();
	}

	const std::vector<std::pair<std::string, std::string> >::const_iterator end() const {
		return mValues.end();
	}

	bool hasKey(const std::string &key, std::string value = "") const;

	const std::string &name() const {
		return mName;
	}

	std::string value(const std::string &key) const;

	void setValue(const std::string& key, const std::string& value);

	bool parse(const std::string& content);

private:
	std::string mName;
	std::vector<std::pair<std::string, std::string> > mValues;
}; 

#endif //INCLUDES_HUE_CONFIG_H
