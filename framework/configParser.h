#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include "joystick.h"
#include <json/json.h>
#include <string>
#include <unordered_map>

namespace configParser {
std::unordered_map<int, int> readButtonMapping(const Json::Value &config);
void initializeJoysticks(const Json::Value &config, Joystick *left, Joystick *right, Joystick *trigger);
bool readRunAutomatically(const Json::Value &config);
} // namespace configParser

#endif // CONFIGPARSER_H
