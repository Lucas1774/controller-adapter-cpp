#include "funcs.h"
#include "gameRegistry.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <windows.h>

using enum Buttons;
using enum ButtonState;
using enum ButtonGroups;

struct State {
    int cardIndex;
};

static constexpr std::array<std::pair<int, int>, 4> CARD_COORDINATES = {
    {{720, CENTER_Y}, CENTER, {1200, CENTER_Y}, {CENTER_X, 825}}};
static constexpr std::array<std::array<Buttons, 4>, 4> CARD_ADJACENCY_MATRIX = {
    {{NONE, PAD_RIGHT, PAD_LEFT, PAD_DOWN}, // left
     {PAD_LEFT, NONE, PAD_RIGHT, PAD_DOWN}, // center
     {PAD_RIGHT, PAD_LEFT, NONE, PAD_DOWN}, // right
     {PAD_LEFT, PAD_UP, PAD_RIGHT, NONE}}}; // reroll

static bool updateAbstractState(const Buttons button, State &state, const std::unordered_map<Buttons, ButtonState> &buttonState, BufferState &bufferState) {
    if (!functions::abstractStateUtils::isBufferFree(buttonState, DEFAULT_SECOND_INPUT_DELAY_MILLIS, DEFAULT_SUBSEQUENT_INPUT_DELAY_MILLIS, button, bufferState)) {
        return false;
    }
    return functions::abstractStateUtils::computeAdjacencyMatrixBasedTarget(CARD_ADJACENCY_MATRIX, state.cardIndex, button);
}

namespace gameRegistry {

void runSwarm(const GameParams &params) {
    auto [buttonMapping, buttonState, running, resScalingX, resScalingY, joystick, leftJoystick, rightJoystick, triggers] = params;
    bool highPrecision = false;
    std::ifstream configFile("swarm/config.json");
    Json::Value specificConfig;
    if (configFile.is_open()) {
        configFile >> specificConfig;
        configFile.close();
    } else {
        std::cerr << "Error opening config file." << std::endl;
        return;
    }
    bool highPrecisionAlwaysOn = specificConfig["high_precision_on_by_default"].asBool();
    const float MAX_RADIUS_HIGH_PRECISION_OFF = specificConfig["default_radius"].asFloat();
    double currentRadius = MAX_RADIUS_HIGH_PRECISION_OFF;

    const auto now = std::chrono::steady_clock::now();
    State state = {
        .cardIndex = 3};
    BufferState bufferState = {
        .lastPressed = now,
        .lastExecuted = now,
        .isUnleashed = false};

    const auto TURBO_INPUTS = std::unordered_set<Buttons>{PAD_LEFT, PAD_RIGHT, PAD_UP, PAD_DOWN, L1, R1};
    const auto INPUT_TO_KEY_TAP = std::unordered_map<Buttons, std::function<int()>>{
        {START, [] { return VK_ESCAPE; }},
        {X, [] { return 'C'; }},
        {Y, [] { return 'O'; }}};
    const auto INPUT_TO_KEY_HOLD = std::unordered_map<Buttons, std::function<int()>>{
        {LEFT_JS_LEFT, [] { return 'A'; }},
        {LEFT_JS_RIGHT, [] { return 'D'; }},
        {LEFT_JS_UP, [] { return 'W'; }},
        {LEFT_JS_DOWN, [] { return 'S'; }},
        {B, [] { return VK_TAB; }},
        {R2, [] { return 'T'; }}};
    const auto INPUT_TO_MOUSE_MOVE = std::unordered_map<Buttons, std::function<std::pair<int, int>()>>{
        {PAD_LEFT, [&state] { return CARD_COORDINATES[state.cardIndex]; }},
        {PAD_RIGHT, [&state] { return CARD_COORDINATES[state.cardIndex]; }},
        {PAD_UP, [&state] { return CARD_COORDINATES[state.cardIndex]; }},
        {PAD_DOWN, [&state] { return CARD_COORDINATES[state.cardIndex]; }},
        {R3, [] { return CENTER; }}};
    const auto INPUT_TO_MOUSE_CLICK = std::unordered_map<Buttons, std::function<int()>>{
        {A, [] { return SDL_BUTTON_LEFT; }}};
    const auto RELEASE_TO_KEY_TAP = std::unordered_map<Buttons, std::function<int()>>{
        {R1, [] { return 'R'; }},
        {L1, [] { return 'E'; }}};
    const auto JOYSTICK_TO_MOUSE_RELATIVE = std::unordered_map<ButtonGroups, std::function<Joystick &()>>{
        {RIGHT_JS, [&rightJoystick]() -> Joystick & { return rightJoystick; }}};
    const auto INPUT_TO_CONDITIONING_LOGIC = std::unordered_map<Buttons, std::function<bool()>>{
        {PAD_LEFT, [&state, &buttonState, &bufferState]() { return updateAbstractState(PAD_LEFT, state, buttonState, bufferState); }},
        {PAD_RIGHT, [&state, &buttonState, &bufferState]() { return updateAbstractState(PAD_RIGHT, state, buttonState, bufferState); }},
        {PAD_UP, [&state, &buttonState, &bufferState]() { return updateAbstractState(PAD_UP, state, buttonState, bufferState); }},
        {PAD_DOWN, [&state, &buttonState, &bufferState]() { return updateAbstractState(PAD_DOWN, state, buttonState, bufferState); }},
        {RIGHT_JS_LEFT, [&highPrecision, &highPrecisionAlwaysOn, &rightJoystick, &currentRadius, resScalingX, resScalingY]() {
             if (highPrecision || highPrecisionAlwaysOn) {
                 return true;
             }
             functions::action::moveMouse(
                 static_cast<int>(round(rightJoystick.x * CENTER_X * currentRadius + CENTER_X)),
                 static_cast<int>(round(rightJoystick.y * CENTER_Y * currentRadius + CENTER_Y)),
                 resScalingX, resScalingY);
             return false;
         }},
        {RIGHT_JS_RIGHT, [&highPrecision, &highPrecisionAlwaysOn, &rightJoystick, &currentRadius, resScalingX, resScalingY]() {
             if (highPrecision || highPrecisionAlwaysOn) {
                 return true;
             }
             functions::action::moveMouse(
                 static_cast<int>(round(rightJoystick.x * CENTER_X * currentRadius + CENTER_X)),
                 static_cast<int>(round(rightJoystick.y * CENTER_Y * currentRadius + CENTER_Y)),
                 resScalingX, resScalingY);
             return false;
         }},
        {RIGHT_JS_UP, [&highPrecision, &highPrecisionAlwaysOn, &rightJoystick, &currentRadius, resScalingX, resScalingY]() {
             if (highPrecision || highPrecisionAlwaysOn) {
                 return true;
             }
             functions::action::moveMouse(
                 static_cast<int>(round(rightJoystick.x * CENTER_X * currentRadius + CENTER_X)),
                 static_cast<int>(round(rightJoystick.y * CENTER_Y * currentRadius + CENTER_Y)),
                 resScalingX, resScalingY);
             return false;
         }},
        {RIGHT_JS_DOWN, [&highPrecision, &highPrecisionAlwaysOn, &rightJoystick, &currentRadius, resScalingX, resScalingY]() {
             if (highPrecision || highPrecisionAlwaysOn) {
                 return true;
             }
             functions::action::moveMouse(
                 static_cast<int>(round(rightJoystick.x * CENTER_X * currentRadius + CENTER_X)),
                 static_cast<int>(round(rightJoystick.y * CENTER_Y * currentRadius + CENTER_Y)),
                 resScalingX, resScalingY);
             return false;
         }}};
    const auto INPUT_TO_LOGIC_BEFORE = std::unordered_map<Buttons, std::function<void()>>{
        {R2, [resScalingX, resScalingY]() { functions::action::moveMouse(CENTER_X, CENTER_Y, resScalingX, resScalingY); return true; }},
        {R3, [&highPrecisionAlwaysOn]() { highPrecisionAlwaysOn = !highPrecisionAlwaysOn; return true; }},
        {L3, [&buttonState, &currentRadius, MAX_RADIUS_HIGH_PRECISION_OFF]() { buttonState[R1] = RELEASED; buttonState[L1] = RELEASED; currentRadius = MAX_RADIUS_HIGH_PRECISION_OFF; }},
        {L2, [&highPrecision]() { highPrecision = true; }},
        {R1, [&currentRadius, &rightJoystick]() { currentRadius += rightJoystick.sensitivity * 0.05f; }},
        {L1, [&currentRadius, &rightJoystick]() { currentRadius += rightJoystick.sensitivity * 0.05f; }}};
    const auto INPUT_TO_LOGIC_AFTER = std::unordered_map<Buttons, std::function<void()>>{
        {A, [&state]() { state.cardIndex = 3; }}};
    const auto RELEASE_TO_LOGIC_BEFORE = std::unordered_map<Buttons, std::function<void()>>{
        {L2, [&highPrecision]() { highPrecision = false; }}};
    const auto RELEASE_TO_LOGIC_AFTER = std::unordered_map<Buttons, std::function<void()>>{
        {R1, [&currentRadius, MAX_RADIUS_HIGH_PRECISION_OFF]() { currentRadius = MAX_RADIUS_HIGH_PRECISION_OFF; }},
        {L1, [&currentRadius, MAX_RADIUS_HIGH_PRECISION_OFF]() { currentRadius = MAX_RADIUS_HIGH_PRECISION_OFF; }}};

    functions::Mappings mappings = {
        .buttonState = buttonState,
        .inputToMouseMove = INPUT_TO_MOUSE_MOVE,
        .inputToMouseClick = INPUT_TO_MOUSE_CLICK,
        .inputToKeyTap = INPUT_TO_KEY_TAP,
        .releaseToKeyTap = RELEASE_TO_KEY_TAP,
        .inputToKeyHold = INPUT_TO_KEY_HOLD,
        .joystickToMouseRelative = JOYSTICK_TO_MOUSE_RELATIVE,
        .inputToConditioningLogic = INPUT_TO_CONDITIONING_LOGIC,
        .inputToLogicBefore = INPUT_TO_LOGIC_BEFORE,
        .releaseToLogicBefore = RELEASE_TO_LOGIC_BEFORE,
        .inputToLogicAfter = INPUT_TO_LOGIC_AFTER,
        .releaseToLogicAfter = RELEASE_TO_LOGIC_AFTER};

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
