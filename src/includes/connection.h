#ifndef INCLUDES_CONNECTION_H
#define INCLUDES_CONNECTION_H

#include <string>

#include <json-c/json.h>

bool downloadJson(std::string url, json_object** output);
bool postJson(std::string url, json_object* input, json_object** output);
bool putJson(std::string url, json_object* input, json_object** output);

#endif //INCLUDES_CONNECTION_H
