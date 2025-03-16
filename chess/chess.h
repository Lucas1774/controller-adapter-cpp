#ifndef CHESS_H
#define CHESS_H

#include <json/json.h>
#include <unordered_map>

namespace chess {
void run(std::unordered_map<int, int> &buttonState,
         const Json::Value &config,
         const int screenWidth,
         const int screenHeight);
} // namespace chess

#endif // CHESS_H
