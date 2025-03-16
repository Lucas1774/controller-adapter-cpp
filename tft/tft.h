#ifndef TFT_H
#define TFT_H

#include <SDL2/SDL.h>
#include <json/json.h>
#include <unordered_map>

namespace tft {
void run(std::unordered_map<int, int> &buttonState,
         const bool &hasTriggers,
         const Json::Value &config,
         const int screenWidth,
         const int screenHeight,
         SDL_Joystick *joystick);
} // namespace tft

#endif // TFT_H
