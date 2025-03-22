#ifndef GAME_REGISTRY_H
#define GAME_REGISTRY_H

#include "constants.h"
#include "joystick.h"
#include <SDL2/SDL.h>
#include <functional>
#include <json/json.h>

namespace gameRegistry {

struct GameParams {
    const std::unordered_map<Uint8, Buttons> &buttonMapping;
    std::unordered_map<Buttons, ButtonState> &buttonState;
    bool running;
    const double resScalingX;
    const double resScalingY;
    SDL_Joystick *joystick;
    Joystick &leftJoystick;
    Joystick &rightJoystick;
    Joystick *triggers;
};

void runGame(const std::string &name,
             const GameParams &params);

void runFrostpunkTwo(const GameParams &params);
void runSwarm(const GameParams &params);
void runChess(const GameParams &params);
void runTft(const GameParams &params);

} // namespace gameRegistry

#endif // GAME_REGISTRY_H
