#ifndef FROSTPUNKTWO_H
#define FROSTPUNKTWO_H

#include <SDL2/SDL.h>
#include <json/json.h>
#include <unordered_map>

namespace frostpunkTwo {
void run(std::unordered_map<int, int> &buttonState,
         const bool &hasTriggers,
         const Json::Value &config,
         const int screenWidth,
         const int screenHeight,
         SDL_Joystick *joystick);
} // namespace frostpunkTwo

#endif // FROSTPUNKTWO_H
