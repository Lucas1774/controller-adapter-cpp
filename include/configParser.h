#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include "constants.h"
#include "joystick.h"
#include <SDL2/SDL.h>
#include <json/json.h>

namespace configParser {
std::unordered_map<Uint8, Buttons> readButtonMapping(const Json::Value &config);
void initializeJoysticks(const Json::Value &config, Joystick &left, Joystick &right, Joystick *&triggers);
bool readRunAutomatically(const Json::Value &config);
} // namespace configParser

#endif // CONFIGPARSER_H
