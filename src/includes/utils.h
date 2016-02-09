#ifndef INCLUDES_UTILS_H
#define INCLUDES_UTILS_H

#include <set>
#include <vector>

void commaListToSet(const std::string& str, std::set<std::string>& v); 
void commaListToVector(const std::string& str, std::vector<std::string>& v); 

#endif // INCLUDES_UTILS_H