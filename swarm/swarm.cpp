#include "swarm.h"
#include "configParser.h"
#include "funcs.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <windows.h>
namespace swarm {

struct State {
    int cardIndex;
};

static constexpr std::array<std::pair<int, int>, 4> CARD_COORDINATES = {
    {{720, CENTER_Y}, CENTER, {1200, CENTER_Y}, {CENTER_X, 825}}};
static constexpr std::array<std::array<int, 4>, 4> CARD_ADJACENCY_MATRIX = {
    {{NONE, PAD_RIGHT, PAD_LEFT, PAD_DOWN}, // left
     {PAD_LEFT, NONE, PAD_RIGHT, PAD_DOWN}, // center
     {PAD_RIGHT, PAD_LEFT, NONE, PAD_DOWN}, // right
     {PAD_LEFT, PAD_UP, PAD_RIGHT, NONE}}}; // reroll

static bool updateAbstractState(const int button, State &state, const std::unordered_map<int, int> &buttonState, BufferState &bufferState) {
    if (!functions::isBufferFree(buttonState, DEFAULT_SECOND_INPUT_DELAY_MILLIS, DEFAULT_SUBSEQUENT_INPUT_DELAY_MILLIS, button, bufferState)) {
        return false;
    }
    return functions::computeAdjacencyMatrixBasedTarget(CARD_ADJACENCY_MATRIX, state.cardIndex, button);
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
    bool highPrecision;
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

    const double resScalingX = screenWidth / 1920.0;
    const double resScalingY = screenHeight / 1080.0;
    const auto now = std::chrono::steady_clock::now();

    State state = {
        .cardIndex = 3};
    BufferState bufferState = {
        .lastPressed = now,
        .lastExecuted = now,
        .isUnleashed = false};

    const auto TURBO_INPUTS = std::unordered_set<int>{PAD_LEFT, PAD_RIGHT, PAD_UP, PAD_DOWN};
    const auto INPUT_TO_KEY_TAP = std::unordered_map<int, std::function<WORD()>>{
        {START, [] { return VK_ESCAPE; }},
        {X, [] { return 'C'; }},
        {Y, [] { return 'O'; }}};
    const auto INPUT_TO_KEY_HOLD = std::unordered_map<int, std::function<WORD()>>{
        {LEFT_JS_LEFT, [] { return 'A'; }},
        {LEFT_JS_RIGHT, [] { return 'D'; }},
        {LEFT_JS_UP, [] { return 'W'; }},
        {LEFT_JS_DOWN, [] { return 'S'; }},
        {B, [] { return VK_TAB; }},
        {R2, [] { return 'T'; }}};
    const auto INPUT_TO_MOUSE_MOVE = std::unordered_map<int, std::function<std::pair<int, int>()>>{
        {PAD_LEFT, [&state] { return CARD_COORDINATES[state.cardIndex]; }},
        {PAD_RIGHT, [&state] { return CARD_COORDINATES[state.cardIndex]; }},
        {PAD_UP, [&state] { return CARD_COORDINATES[state.cardIndex]; }},
        {PAD_DOWN, [&state] { return CARD_COORDINATES[state.cardIndex]; }},
        {R3, [] { return CENTER; }}};
    const auto INPUT_TO_MOUSE_CLICK = std::unordered_map<int, std::function<int()>>{
        {A, [] { return SDL_BUTTON_LEFT; }}};
    const auto RELEASE_TO_KEY_TAP = std::unordered_map<int, std::function<WORD()>>{
        {R1, [] { return 'R'; }},
        {L1, [] { return 'E'; }}};
    const auto INPUT_TO_LOGIC_BEFORE = std::unordered_map<int, std::function<bool()>>{
        {R2, [resScalingX, resScalingY]() { functions::moveMouse(CENTER_X, CENTER_Y, resScalingX, resScalingY); return true; }},
        {R3, [&highPrecisionAlwaysOn]() { highPrecisionAlwaysOn = !highPrecisionAlwaysOn; return true; }},
        {PAD_LEFT, [&state, &buttonState, &bufferState]() { return updateAbstractState(PAD_LEFT, state, buttonState, bufferState); }},
        {PAD_RIGHT, [&state, &buttonState, &bufferState]() { return updateAbstractState(PAD_RIGHT, state, buttonState, bufferState); }},
        {PAD_UP, [&state, &buttonState, &bufferState]() { return updateAbstractState(PAD_UP, state, buttonState, bufferState); }},
        {PAD_DOWN, [&state, &buttonState, &bufferState]() { return updateAbstractState(PAD_DOWN, state, buttonState, bufferState); }}};
    const auto INPUT_TO_LOGIC_AFTER = std::unordered_map<int, std::function<void()>>{{A, [&state]() { state.cardIndex = 3; }}};
    const auto RELEASE_TO_LOGIC_AFTER = std::unordered_map<int, std::function<void()>>{
        {R1, [&currentRadius, MAX_RADIUS_HIGH_PRECISION_OFF]() { currentRadius = MAX_RADIUS_HIGH_PRECISION_OFF; }},
        {L1, [&currentRadius, MAX_RADIUS_HIGH_PRECISION_OFF]() { currentRadius = MAX_RADIUS_HIGH_PRECISION_OFF; }}};

    functions::Mappings mappings = {
        .buttonState = buttonState,
        .input_to_mouse_move = INPUT_TO_MOUSE_MOVE,
        .input_to_mouse_click = INPUT_TO_MOUSE_CLICK,
        .input_to_key_tap = INPUT_TO_KEY_TAP,
        .release_to_key_tap = RELEASE_TO_KEY_TAP,
        .input_to_key_hold = INPUT_TO_KEY_HOLD,
        .input_to_logic_before = INPUT_TO_LOGIC_BEFORE,
        .input_to_logic_after = INPUT_TO_LOGIC_AFTER,
        .release_to_logic_after = RELEASE_TO_LOGIC_AFTER};

    try {
        auto lastUpdateTime = std::chrono::steady_clock::now();
        while (true) {
            const auto loopStartTime = std::chrono::steady_clock::now();
            std::vector<SDL_Event> events;
            SDL_Event eventBuffer;
            while (SDL_PollEvent(&eventBuffer)) {
                events.push_back(eventBuffer);
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
                highPrecision = buttonState[L2] == PRESSED || buttonState[L2] == JUST_PRESSED;
                if (buttonState[R1] == PRESSED || buttonState[L1] == PRESSED) {
                    currentRadius += rightJoystick.sensitivity * 0.05f;
                }
                if (buttonState[L3] == JUST_PRESSED) {
                    buttonState[R1] = RELEASED;
                    buttonState[L1] = RELEASED;
                    currentRadius = MAX_RADIUS_HIGH_PRECISION_OFF;
                }

                // action
                functions::runMappings(mappings, resScalingX, resScalingY, TURBO_INPUTS);

                if (rightJoystick.isXActive || rightJoystick.isYActive) {
                    if (highPrecisionAlwaysOn || highPrecision) {
                        if (std::chrono::steady_clock::now() - lastUpdateTime > std::chrono::milliseconds(MILLIS_PER_FRAME)) {
                            functions::moveMouseRelative(
                                static_cast<int>(round(rightJoystick.x * rightJoystick.sensitivity * 100)),
                                static_cast<int>(round(rightJoystick.y * rightJoystick.sensitivity * 100)),
                                resScalingX, resScalingY);
                            lastUpdateTime = std::chrono::steady_clock::now();
                        }
                    } else {
                        functions::moveMouse(
                            static_cast<int>(round(rightJoystick.x * CENTER_X * currentRadius + CENTER_X)),
                            static_cast<int>(round(rightJoystick.y * CENTER_Y * currentRadius + CENTER_Y)),
                            resScalingX, resScalingY);
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(std::max(
                10000 - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - loopStartTime).count(), 0LL)));
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

} // namespace swarm
