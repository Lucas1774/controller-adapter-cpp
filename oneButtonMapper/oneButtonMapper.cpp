#include "funcs.h"
#include "gameRegistry.h"
#include <windows.h>

namespace gameRegistry {

void runOneButtonMapper(const GameParams &params) {
    auto [buttonMapping, buttonState, running, resScalingX, resScalingY, joystick, leftJoystick, rightJoystick, triggers] = params;

    const auto INPUT_TO_KEY_HOLD = std::unordered_map<Buttons, std::function<int()>>{
        {R3, [] { return VK_TAB; }}};

    functions::Mappings mappings = {
        .buttonState = buttonState,
        .inputToKeyHold = INPUT_TO_KEY_HOLD};

    const std::unordered_set<Buttons> TURBO_INPUTS = {};
    functions::GameParams gameParams = {
        .buttonMapping = buttonMapping,
        .buttonState = buttonState,
        .joystick = joystick,
        .leftJoystick = leftJoystick,
        .rightJoystick = rightJoystick,
        .triggers = triggers,
        .mappings = mappings,
        .resScalingX = resScalingX,
        .resScalingY = resScalingY,
        .running = running,
        .turboInputs = TURBO_INPUTS};

    functions::run(gameParams);
}

} // namespace gameRegistry
