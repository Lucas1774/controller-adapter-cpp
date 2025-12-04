#include "funcs.h"
#include "gameRegistry.h"
#include <cmath>
#include <windows.h>

namespace gameRegistry {

void runMegabonk(const GameParams &params) {
    auto [buttonMapping, buttonState, running, resScalingX, resScalingY, joystick, leftJoystick, rightJoystick, triggers] = params;

    const auto TURBO_INPUTS = std::unordered_set<Buttons>{R1, LEFT_JS_LEFT, LEFT_JS_RIGHT};
    const auto INPUT_TO_KEY_TAP = std::unordered_map<Buttons, std::function<int()>>{{R1, [] { return VK_SPACE; }}};
    const auto INPUT_TO_LOGIC_BEFORE = std::unordered_map<Buttons, std::function<void()>>{
        {LEFT_JS_LEFT, [resScalingX, resScalingY, &leftJoystick]() { functions::action::moveMouseRelative(
                                                                         static_cast<int>(round(leftJoystick.x * leftJoystick.x * leftJoystick.sensitivity * 100 * (leftJoystick.x / std::abs(leftJoystick.x)))),
                                                                         0, resScalingX, resScalingY); }},
        {LEFT_JS_RIGHT, [resScalingX, resScalingY, &leftJoystick]() { functions::action::moveMouseRelative(
                                                                          static_cast<int>(round(leftJoystick.x * leftJoystick.x * leftJoystick.sensitivity * 100 * (leftJoystick.x / std::abs(leftJoystick.x)))),
                                                                          0, resScalingX, resScalingY); }}};

    functions::Mappings mappings = {
        .buttonState = buttonState,
        .inputToKeyTap = INPUT_TO_KEY_TAP,
        .inputToLogicBefore = INPUT_TO_LOGIC_BEFORE};

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
