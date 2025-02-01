#include "frostpunkTwo.h"
#include "configParser.h"
#include "funcs.h"
#include "joystick.h"
#include <cmath>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <windows.h>

namespace frostpunkTwo {

struct State {
    int speedRow;
    int speedColumn;
    int buildMenuRow;
    int buildMenuColumn;
    int overlayRow;
    int overlayColumn;
    int shopRow;
    int shopColumn;
    std::pair<int, int> mouseTarget;
};

// TODO: grab real coordinates
constexpr std::array<std::array<std::pair<int, int>, 5>, 2> SHOP_COORDINATES = {
    {{{{503, 982}, {714, 982}, {923, 985}, {1151, 984}, {1348, 987}}},
     {{{503, 982}, {714, 982}, {923, 985}, {1151, 984}, {1348, 987}}}}};
constexpr std::array<std::array<int, 4>, 1> BUILD_MENU_KEYS = {
    {{{'N', 'B', 'H', 'X'}}}};
constexpr std::array<std::array<int, 3>, 1> SPEED_KEYS = {
    {{{'1', '2', '3'}}}};
constexpr std::array<std::array<int, 6>, 1> OVERLAY_KEYS = {
    {{{VK_LMENU, '4', '5', '6', '7', '8'}}}};
static bool updateAbstractState(const int button, State &state, BufferState &bufferState, const Functions &functions) {
    static int dummyTarget = -1;
    static const char dummyButton = -1;
    static const std::map<int, std::function<bool()>> keyToFunction = {
        {L1, [&functions, &state]() { return functions.computeGridBasedTarget(BUILD_MENU_KEYS, dummyTarget, state.buildMenuRow, state.buildMenuColumn, dummyButton); }},
        {R1, [&functions, &state]() { return functions.computeGridBasedTarget(SPEED_KEYS, dummyTarget, state.speedRow, state.speedColumn, dummyButton); }},
        {Y, [&functions, &state]() { return functions.computeGridBasedTarget(OVERLAY_KEYS, dummyTarget, state.overlayRow, state.overlayColumn, dummyButton); }}};

    if (auto it = keyToFunction.find(button); it != keyToFunction.end()) {
        return it->second();
    }
    if (!functions.isBufferFree(DEFAULT_SECOND_INPUT_DELAY_MILLIS, DEFAULT_SUBSEQUENT_INPUT_DELAY_MILLIS, button, bufferState)) {
        return false;
    }
    return functions.computeGridBasedTarget(SHOP_COORDINATES, state.mouseTarget, state.shopRow, state.speedColumn, button);
}

void run(std::unordered_map<int, int> &buttonState,
         const bool &hasTriggers,
         const Json::Value &config,
         const int screenWidth,
         const int screenHeight,
         SDL_Joystick *joystick) {
    Joystick leftJoystick, rightJoystick, triggers;
    configParser::initializeJoysticks(config, &leftJoystick, &rightJoystick, hasTriggers ? &triggers : nullptr);
    std::unordered_map<int, int> buttonMapping = configParser::readButtonMapping(config);
    bool running = configParser::readRunAutomatically(config);

    Functions functions;
    const double resScalingX = screenWidth / 1920.0;
    const double resScalingY = screenHeight / 1080.0;
    const auto now = std::chrono::steady_clock::now();

    State state = {
        .speedRow = 0,
        .speedColumn = 0,
        .buildMenuRow = 0,
        .buildMenuColumn = 0,
        .overlayRow = 0,
        .overlayColumn = 0,
        .shopRow = 0,
        .shopColumn = 0,
        .mouseTarget = {}};
    BufferState bufferState = {
        .lastPressed = now,
        .lastExecuted = now,
        .isUnleashed = false};

    const auto TURBO_INPUTS = std::unordered_set<int>{PAD_LEFT, PAD_RIGHT, PAD_UP, PAD_DOWN};
    const auto INPUT_TO_KEY_TAP = std::unordered_map<int, std::function<WORD()>>{
        {R1, [&state]() { return SPEED_KEYS[state.speedRow][state.speedColumn]; }},
        {L1, [&state]() { return BUILD_MENU_KEYS[state.buildMenuRow][state.buildMenuColumn]; }},
        {R2, []() { return 'E'; }},
        {L2, []() { return 'Q'; }},
        {R3, []() { return 'C'; }},
        {L3, []() { return 'R'; }},
        {SELECT, []() { return 'V'; }},
        {START, []() { return VK_ESCAPE; }},
        {X, []() { return VK_SPACE; }}};
    const auto INPUT_TO_KEY_HOLD = std::unordered_map<int, std::function<WORD()>>{
        {LEFT_JS_LEFT, []() { return 'A'; }},
        {LEFT_JS_RIGHT, []() { return 'D'; }},
        {LEFT_JS_UP, []() { return 'W'; }},
        {LEFT_JS_DOWN, []() { return 'S'; }},
        {Y, [&state]() { return OVERLAY_KEYS[state.overlayRow][state.overlayColumn]; }}};
    const auto INPUT_TO_MOUSE_CLICK = std::unordered_map<int, std::function<int()>>{
        {A, []() { return SDL_BUTTON_LEFT; }},
        {B, []() { return SDL_BUTTON_RIGHT; }}};
    const auto INPUT_TO_MOUSE_MOVE = std::unordered_map<int, std::function<std::pair<int, int>()>>{
        {PAD_LEFT, [&state]() { return state.mouseTarget; }},
        {PAD_RIGHT, [&state]() { return state.mouseTarget; }},
        {PAD_UP, [&state]() { return state.mouseTarget; }},
        {PAD_DOWN, [&state]() { return state.mouseTarget; }}};
    const auto INPUT_TO_LOGIC_AFTER = std::unordered_map<int, std::function<bool()>>{
        {PAD_LEFT, [&functions, &state, &bufferState]() { return updateAbstractState(PAD_LEFT, state, bufferState, functions); }},
        {PAD_RIGHT, [&functions, &state, &bufferState]() { return updateAbstractState(PAD_RIGHT, state, bufferState, functions); }},
        {PAD_UP, [&functions, &state, &bufferState]() { return updateAbstractState(PAD_UP, state, bufferState, functions); }},
        {PAD_DOWN, [&functions, &state, &bufferState]() { return updateAbstractState(PAD_DOWN, state, bufferState, functions); }},
        {L1, [&functions, &state, &bufferState]() { return updateAbstractState(L1, state, bufferState, functions); }},
        {R1, [&functions, &state, &bufferState]() { return updateAbstractState(R1, state, bufferState, functions); }}};
    const auto RELEASE_TO_LOGIC_AFTER = std::unordered_map<int, std::function<bool()>>{
        {Y, [&functions, &state, &bufferState]() { return updateAbstractState(Y, state, bufferState, functions); }}};

    functions.setMaps(&buttonState, &INPUT_TO_MOUSE_MOVE, nullptr, &INPUT_TO_MOUSE_CLICK, nullptr, nullptr, nullptr, &INPUT_TO_KEY_TAP, nullptr, &INPUT_TO_KEY_HOLD, nullptr, &INPUT_TO_LOGIC_AFTER, nullptr, &RELEASE_TO_LOGIC_AFTER);

    try {
        auto lastUpdateTime = std::chrono::steady_clock::now();
        while (true) {
            auto loopStartTime = std::chrono::steady_clock::now();
            std::vector<SDL_Event> events;
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                events.push_back(event);
            }
            if (!running) {
                functions.listenToRunEvent(events, buttonMapping, running);
            } else {
                // state
                functions.updateNonAnalogState(events, buttonMapping);
                if (buttonState[ACTIVATE] == JUST_PRESSED) {
                    running = false;
                    continue;
                }
                functions.updateJoystickAsDigital(joystick, leftJoystick, LEFT_JS);
                functions.updateJoystickAsAnalog(joystick, rightJoystick, RIGHT_JS);
                if (hasTriggers) {
                    functions.updateJoystickAsDigital(joystick, triggers, TRIGGERS);
                }
                if (buttonState[X] == JUST_PRESSED) {
                    state.speedRow = 0;
                    state.speedColumn = 0;
                }
                if (buttonState[A] == JUST_PRESSED || buttonState[B] == JUST_PRESSED) {
                    state.overlayRow = 0;
                    state.overlayColumn = 0;
                    state.buildMenuRow = 0;
                    state.buildMenuColumn = 0;
                }

                // action
                for (const auto &[input, _] : INPUT_TO_MOUSE_MOVE) {
                    if (TURBO_INPUTS.find(input) != TURBO_INPUTS.end()) {
                        functions.handleToMouseAbsoluteMove(input, PRESSED, resScalingX, resScalingY);
                    }
                    functions.handleToMouseAbsoluteMove(input, JUST_PRESSED, resScalingX, resScalingY);
                }
                for (const auto &[input, _] : INPUT_TO_MOUSE_CLICK) {
                    functions.handleToClick(input, JUST_PRESSED);
                }
                for (const auto &[input, _] : INPUT_TO_KEY_TAP) {
                    functions.handleToKeyTap(input, JUST_PRESSED);
                }
                for (const auto &[input, _] : INPUT_TO_KEY_HOLD) {
                    functions.handleToKeyHold(input);
                }

                if ((rightJoystick.isXActive || rightJoystick.isYActive) && std::chrono::steady_clock::now() - lastUpdateTime > std::chrono::milliseconds(MILLIS_PER_FRAME)) {
                    functions.moveMouseRelative(
                        static_cast<int>(round(rightJoystick.x * rightJoystick.sensitivity * 100)),
                        static_cast<int>(round(rightJoystick.y * rightJoystick.sensitivity * 100)),
                        resScalingX, resScalingY);
                    lastUpdateTime = std::chrono::steady_clock::now();
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(std::max(
                10000 - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - loopStartTime).count(), 0LL)));
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

} // namespace frostpunkTwo
