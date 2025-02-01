#ifndef TFT_H
#define TFT_H

#include "funcs.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <json/json.h>
#include <unordered_map>

constexpr int NONE = -1;

namespace tft {
void run(std::unordered_map<int, int> &buttonState,
         const bool &hasTriggers,
         const Json::Value &config,
         const int screenWidth,
         const int screenHeight,
         SDL_Joystick *joystick);
enum class MouseMovementWithPadMode {
    BOARD = 0,
    ITEMS,
    SHOP,
    CARDS,
    LOCK,
    FREE,
};
struct State {
    int boardRow;
    int boardColumn;
    int itemColumn;
    int itemRow;
    int shopIndex;
    int cardRow;
    int cardColumn;
    int lockIndex;
    MouseMovementWithPadMode mode;
    MouseMovementWithPadMode previous_mode;
    std::pair<int, int> mouse_target;
};
bool updateAbstractState(const int button, State &state, BufferState &bufferState, const double res_scaling_x, const double res_scaling_y, const Functions &functions);
void updateAbstractState(State &state, const double res_scaling_x, const double res_scaling_y, const Functions &functions);
} // namespace tft

#endif // TFT_H
