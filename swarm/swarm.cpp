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
    std::pair<int, int> mouseTarget;
};

static constexpr std::array<std::pair<int, int>, 4> CARD_COORDINATES = {
    {{720, CENTER_Y}, CENTER, {1200, CENTER_Y}, {CENTER_X, 825}}};

static constexpr std::array<std::array<int, 4>, 4> CARD_ADJACENCY_MATRIX = {
    {{NONE, PAD_RIGHT, PAD_LEFT, PAD_DOWN}, // left
     {PAD_LEFT, NONE, PAD_RIGHT, PAD_DOWN}, // center
     {PAD_RIGHT, PAD_LEFT, NONE, PAD_DOWN}, // right
     {PAD_LEFT, PAD_UP, PAD_RIGHT, NONE}}}; // reroll

static bool updateAbstractState(const int button, State &state, BufferState &bufferState, const double resScalingX, const double resScalingY, const Functions &functions) {
    if (!functions.isBufferFree(200, 50, button, bufferState)) {
        return false;
    }
    return functions.computeAdjacencyMatrixBasedMouseTarget(CARD_ADJACENCY_MATRIX, CARD_COORDINATES, state.mouseTarget, state.cardIndex, button, resScalingX, resScalingY);
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

    Functions functions;
    const double resScalingX = screenWidth / 1920.0;
    const double resScalingY = screenHeight / 1080.0;
    const auto now = std::chrono::steady_clock::now();
    std::pair<int, int> center = CENTER;

    State state = {
        .cardIndex = 1,
        .mouseTarget = {}};
    BufferState bufferState = {
        .last_pressed = now,
        .last_executed = now,
        .is_unleashed = false};

    const auto TURBO_INPUTS = std::unordered_set<int>{PAD_LEFT, PAD_RIGHT, PAD_UP, PAD_DOWN};
    const auto INPUT_TO_KEY_TAP = std::unordered_map<int, WORD>{{START, VK_ESCAPE}, {X, 'C'}, {Y, 'O'}};
    const auto INPUT_TO_KEY_HOLD = std::unordered_map<int, WORD>{
        {LEFT_JS_LEFT, 'A'},
        {LEFT_JS_RIGHT, 'D'},
        {LEFT_JS_UP, 'W'},
        {LEFT_JS_DOWN, 'S'},
        {B, VK_TAB},
        {R2, 'T'},
    };
    const auto INPUT_TO_MOUSE_MOVE = std::unordered_map<int, std::pair<int, int> *>{
        {PAD_LEFT, &state.mouseTarget},
        {PAD_RIGHT, &state.mouseTarget},
        {PAD_UP, &state.mouseTarget},
        {PAD_DOWN, &state.mouseTarget},
        {R3, &center},
    };
    const auto INPUT_TO_MOUSE_CLICK = std::unordered_map<int, int>{{A, SDL_BUTTON_LEFT}};
    const auto RELEASE_TO_KEY_TAP = std::unordered_map<int, WORD>{{R1, 'R'}, {L1, 'E'}};
    const auto INPUT_TO_LOGIC_BEFORE = std::unordered_map<int, std::function<bool()>>{
        {R2, [&]() { functions.moveMouse(CENTER_X, CENTER_Y, resScalingX, resScalingY); return true; }},
        {R3, [&]() { highPrecisionAlwaysOn = !highPrecisionAlwaysOn; return true; }},
        {PAD_LEFT, [&]() { return updateAbstractState(PAD_LEFT, state, bufferState, resScalingX, resScalingY, functions); }},
        {PAD_RIGHT, [&]() { return updateAbstractState(PAD_RIGHT, state, bufferState, resScalingX, resScalingY, functions); }},
        {PAD_UP, [&]() { return updateAbstractState(PAD_UP, state, bufferState, resScalingX, resScalingY, functions); }},
        {PAD_DOWN, [&]() { return updateAbstractState(PAD_DOWN, state, bufferState, resScalingX, resScalingY, functions); }},
    };
    const auto RELEASE_TO_LOGIC_AFTER = std::unordered_map<int, std::function<bool()>>{
        {R1, [&]() { currentRadius = MAX_RADIUS_HIGH_PRECISION_OFF; return true; }},
        {L1, [&]() { currentRadius = MAX_RADIUS_HIGH_PRECISION_OFF; return true; }},
    };

    functions.setMaps(&buttonState, &INPUT_TO_MOUSE_MOVE, nullptr, &INPUT_TO_MOUSE_CLICK, nullptr, nullptr, nullptr, &INPUT_TO_KEY_TAP, &RELEASE_TO_KEY_TAP, &INPUT_TO_KEY_HOLD, &INPUT_TO_LOGIC_BEFORE, nullptr, nullptr, &RELEASE_TO_LOGIC_AFTER);

    try {
        auto lastUpdateTime = std::chrono::steady_clock::now();
        while (true) {
            auto loopStartTime = std::chrono::steady_clock::now();
            std::vector<SDL_Event> events;
            SDL_Event eventBuffer;
            while (SDL_PollEvent(&eventBuffer)) {
                events.push_back(eventBuffer);
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
                highPrecision = buttonState[L2] == PRESSED || buttonState[L2] == JUST_PRESSED;
                if (buttonState[R1] == PRESSED || buttonState[L1] == PRESSED) {
                    currentRadius += rightJoystick.sensitivity * 0.05f;
                }
                if (buttonState[L3] == JUST_PRESSED) {
                    buttonState[R1] = RELEASED;
                    buttonState[L1] = RELEASED;
                }

                // action
                for (const auto &[input, _] : INPUT_TO_KEY_TAP) {
                    functions.handleToKeyTap(input, JUST_PRESSED);
                }
                for (const auto &[input, _] : INPUT_TO_KEY_HOLD) {
                    functions.handleToKeyHold(input);
                }
                for (const auto &[input, _] : INPUT_TO_MOUSE_MOVE) {
                    if (TURBO_INPUTS.find(input) != TURBO_INPUTS.end()) {
                        functions.handleToMouseAbsoluteMove(input, PRESSED, resScalingX, resScalingY);
                    }
                    functions.handleToMouseAbsoluteMove(input, JUST_PRESSED, resScalingX, resScalingY);
                }
                for (const auto &[input, _] : RELEASE_TO_KEY_TAP) {
                    functions.handleToKeyTap(input, JUST_RELEASED);
                }
                for (const auto &[input, _] : INPUT_TO_MOUSE_CLICK) {
                    functions.handleToClick(input, JUST_PRESSED);
                }

                if (rightJoystick.isXActive || rightJoystick.isYActive) {
                    if (highPrecisionAlwaysOn || highPrecision) {
                        if (std::chrono::steady_clock::now() - lastUpdateTime > std::chrono::milliseconds(16)) {
                            functions.moveMouseRelative(
                                static_cast<int>(round(rightJoystick.x * rightJoystick.sensitivity * 100)),
                                static_cast<int>(round(rightJoystick.y * rightJoystick.sensitivity * 100)),
                                resScalingX, resScalingY);
                            lastUpdateTime = std::chrono::steady_clock::now();
                        }
                    } else {
                        functions.moveMouse(
                            static_cast<int>(round(rightJoystick.x * CENTER_X * currentRadius + CENTER_X)),
                            static_cast<int>(round(rightJoystick.y * CENTER_Y * currentRadius + CENTER_Y)),
                            resScalingX, resScalingY);
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(std::max(10000 - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - loopStartTime).count(), 0LL)));
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}
} // namespace swarm
