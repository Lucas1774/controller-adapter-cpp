#include "funcs.h"
#include "gameRegistry.h"
#include <windows.h>

using enum Buttons;
using enum ButtonGroups;

namespace gameRegistry {

void runSlayTheSpire(const GameParams &params) {
    auto [buttonMapping, buttonState, running, resScalingX, resScalingY, joystick, leftJoystick, rightJoystick, triggers] = params;

    const auto TURBO_INPUTS = std::unordered_set<Buttons>{RIGHT_JS_UP, RIGHT_JS_DOWN};
    const auto INPUT_TO_MOUSE_SCROLL = std::unordered_map<Buttons, std::function<int()>>{
        {RIGHT_JS_UP, [] { return 1; }},
        {RIGHT_JS_DOWN, [] { return -1; }}};
    const auto INPUT_TO_LOGIC_BEFORE = std::unordered_map<Buttons, std::function<void()>>{
        {R3, [resScalingX, resScalingY] {
             // let the game recognize kbm mode before clicking
             functions::action::moveMouse(45, 1035, resScalingX, resScalingY);
             Sleep(100);
             functions::action::moveMouse(45, 1035, resScalingX, resScalingY);
             functions::action::click(SDL_BUTTON_LEFT);
         }},
        {RIGHT_JS_UP, [resScalingX, resScalingY] {
             // need to focus on the cards panel
             functions::action::moveMouse(CENTER_X, CENTER_Y, resScalingX, resScalingY);
         }},
        {RIGHT_JS_DOWN, [resScalingX, resScalingY] {
             functions::action::moveMouse(CENTER_X, CENTER_Y, resScalingX, resScalingY);
         }}};

    functions::Mappings mappings = {
        .buttonState = buttonState,
        .inputToMouseScroll = INPUT_TO_MOUSE_SCROLL,
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
