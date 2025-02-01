#ifndef CHESS_H
#define CHESS_H

#include "funcs.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <json/json.h>
#include <unordered_map>

namespace chess {
void run(std::unordered_map<int, int> &buttonState,
         const Json::Value &config,
         const int screenWidth,
         const int screenHeight);
enum class Mode {
    BOARD,
    RESIGN,
    DRAW,
};
struct State {
    int boardRow;
    int boardColumn;
    int resignIndex;
    int drawIndex;
    Mode mode;
    std::pair<int, int> mouse_target;
};
bool updateAbstractState(const int button, State &state, BufferState &BufferState, double res_scaling_x, const double res_scaling_y, const Functions &functions);
} // namespace chess

#endif // CHESS_H
