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

static void updateAbstractState(const int button, State &state, const std::unordered_map<int, int> &buttonState, BufferState &bufferState) {
    static const std::map<int, std::function<bool()>> keyToFunction = {
        {L1, [&state]() {
             return functions::computeGridBasedTarget(BUILD_MENU_KEYS.size(), BUILD_MENU_KEYS[0].size(), state.buildMenuRow, state.buildMenuColumn, PAD_RIGHT);
         }},
        {R1, [&state]() {
             return functions::computeGridBasedTarget(SPEED_KEYS.size(), SPEED_KEYS[0].size(), state.speedRow, state.speedColumn, PAD_RIGHT);
         }},
        {Y, [&state]() {
             return functions::computeGridBasedTarget(OVERLAY_KEYS.size(), OVERLAY_KEYS[0].size(), state.overlayRow, state.overlayColumn, PAD_RIGHT);
         }}};

    if (const auto it = keyToFunction.find(button); it != keyToFunction.end()) {
        it->second();
    } else if (functions::isBufferFree(buttonState, DEFAULT_SECOND_INPUT_DELAY_MILLIS, DEFAULT_SUBSEQUENT_INPUT_DELAY_MILLIS, button, bufferState)) {
        functions::computeGridBasedTarget(SHOP_COORDINATES.size(), SHOP_COORDINATES[0].size(), state.shopRow, state.shopColumn, button);
    }
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
        .shopColumn = 0};
    BufferState bufferState = {
        .lastPressed = now,
        .lastExecuted = now,
        .isUnleashed = false};

    const auto TURBO_INPUTS = std::unordered_set<int>{PAD_LEFT, PAD_RIGHT, PAD_UP, PAD_DOWN};
    const auto INPUT_TO_KEY_TAP = std::unordered_map<int, std::function<WORD()>>{
        {R1, [&state]() { return SPEED_KEYS[state.speedRow][state.speedColumn]; }},
        {L1, [&state]() { return BUILD_MENU_KEYS[state.buildMenuRow][state.buildMenuColumn]; }},
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
        {R2, []() { return 'E'; }},
        {L2, []() { return 'Q'; }},
        {Y, [&state]() { return OVERLAY_KEYS[state.overlayRow][state.overlayColumn]; }}};
    const auto INPUT_TO_MOUSE_CLICK = std::unordered_map<int, std::function<int()>>{
        {A, []() { return SDL_BUTTON_LEFT; }},
        {B, []() { return SDL_BUTTON_RIGHT; }}};
    const auto INPUT_TO_MOUSE_MOVE = std::unordered_map<int, std::function<std::pair<int, int>()>>{
        {PAD_LEFT, [&state]() { return SHOP_COORDINATES[state.shopRow][state.shopColumn]; }},
        {PAD_RIGHT, [&state]() { return SHOP_COORDINATES[state.shopRow][state.shopColumn]; }},
        {PAD_UP, [&state]() { return SHOP_COORDINATES[state.shopRow][state.shopColumn]; }},
        {PAD_DOWN, [&state]() { return SHOP_COORDINATES[state.shopRow][state.shopColumn]; }}};
    const auto INPUT_TO_LOGIC_AFTER = std::unordered_map<int, std::function<void()>>{
        {PAD_LEFT, [&state, &buttonState, &bufferState]() { updateAbstractState(PAD_LEFT, state, buttonState, bufferState); }},
        {PAD_RIGHT, [&state, &buttonState, &bufferState]() { updateAbstractState(PAD_RIGHT, state, buttonState, bufferState); }},
        {PAD_UP, [&state, &buttonState, &bufferState]() { updateAbstractState(PAD_UP, state, buttonState, bufferState); }},
        {PAD_DOWN, [&state, &buttonState, &bufferState]() { updateAbstractState(PAD_DOWN, state, buttonState, bufferState); }},
        {L1, [&state, &buttonState, &bufferState]() { updateAbstractState(L1, state, buttonState, bufferState); }},
        {R1, [&state, &buttonState, &bufferState]() { updateAbstractState(R1, state, buttonState, bufferState); }}};
    const auto RELEASE_TO_LOGIC_AFTER = std::unordered_map<int, std::function<void()>>{
        {Y, [&state, &buttonState, &bufferState]() { updateAbstractState(Y, state, buttonState, bufferState); }}};

    functions::Mappings mappings = {
        .buttonState = buttonState,
        .input_to_mouse_move = INPUT_TO_MOUSE_MOVE,
        .input_to_mouse_click = INPUT_TO_MOUSE_CLICK,
        .input_to_key_tap = INPUT_TO_KEY_TAP,
        .input_to_key_hold = INPUT_TO_KEY_HOLD,
        .input_to_logic_after = INPUT_TO_LOGIC_AFTER,
        .release_to_logic_after = RELEASE_TO_LOGIC_AFTER};

    try {
        auto lastUpdateTime = std::chrono::steady_clock::now();
        while (true) {
            const auto loopStartTime = std::chrono::steady_clock::now();
            std::vector<SDL_Event> events;
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                events.push_back(event);
            }
            if (!running) {
                functions::listenToRunEvent(events, buttonMapping, running);
            } else {
                // state
                functions::updateNonAnalogState(buttonState, events, buttonMapping);
                if (buttonState[ACTIVATE] == JUST_PRESSED) {
                    running = false;
                    continue;
                }
                functions::updateJoystickAsDigital(buttonState, joystick, leftJoystick, LEFT_JS);
                functions::updateJoystickAsAnalog(joystick, rightJoystick, RIGHT_JS);
                if (hasTriggers) {
                    functions::updateJoystickAsDigital(buttonState, joystick, triggers, TRIGGERS);
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
                        functions::handleToMouseAbsoluteMove(mappings, input, PRESSED, resScalingX, resScalingY);
                    }
                    functions::handleToMouseAbsoluteMove(mappings, input, JUST_PRESSED, resScalingX, resScalingY);
                }
                for (const auto &[input, _] : INPUT_TO_MOUSE_CLICK) {
                    functions::handleToClick(mappings, input, JUST_PRESSED);
                }
                for (const auto &[input, _] : INPUT_TO_KEY_TAP) {
                    functions::handleToKeyTap(mappings, input, JUST_PRESSED);
                }
                for (const auto &[input, _] : INPUT_TO_KEY_HOLD) {
                    functions::handleToKeyHold(mappings, input);
                }

                if ((rightJoystick.isXActive || rightJoystick.isYActive) && std::chrono::steady_clock::now() - lastUpdateTime > std::chrono::milliseconds(MILLIS_PER_FRAME)) {
                    functions::moveMouseRelative(
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
