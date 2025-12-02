#include "funcs.h"
#include "gameRegistry.h"
#include <windows.h>

namespace gameRegistry {

void runMegabonk(const GameParams &params) {
    auto [buttonMapping, buttonState, running, resScalingX, resScalingY, joystick, leftJoystick, rightJoystick, triggers] = params;

    const auto TURBO_INPUTS = std::unordered_set<Buttons>{R1};
    const auto INPUT_TO_KEY_TAP = std::unordered_map<Buttons, std::function<int()>>{{R1, [] { return VK_SPACE; }}};
    const auto INPUT_TO_KEY_HOLD = std::unordered_map<Buttons, std::function<int()>>{{R1, [] { return 'W'; }}};
    const auto JOYSTICK_TO_MOUSE_RELATIVE = std::unordered_map<ButtonGroups, std::function<Joystick &()>>{
        {LEFT_JS, [&leftJoystick]() -> Joystick & { return leftJoystick; }}};
    const auto INPUT_TO_CONDITIONING_LOGIC = std::unordered_map<Buttons, std::function<bool()>>{
        {LEFT_JS_LEFT, [&buttonState]() { return PRESSED_STATES.contains(buttonState.at(R1)); }},
        {LEFT_JS_RIGHT, [&buttonState]() { return PRESSED_STATES.contains(buttonState.at(R1)); }},
        {LEFT_JS_UP, []() { return false; }},
        {LEFT_JS_DOWN, []() { return false; }}};

    functions::Mappings mappings = {
        .buttonState = buttonState,
        .inputToKeyTap = INPUT_TO_KEY_TAP,
        .inputToKeyHold = INPUT_TO_KEY_HOLD,
        .joystickToMouseRelative = JOYSTICK_TO_MOUSE_RELATIVE,
        .inputToConditioningLogic = INPUT_TO_CONDITIONING_LOGIC};

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
