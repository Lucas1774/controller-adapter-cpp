#include "swarm.h"
#include "constants.h"
#include "funcs.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>
#include <windows.h>

namespace swarm {
void run(std::unordered_map<int, int> &buttonState,
         const bool &hasTriggers,
         const Json::Value &config,
         const int screenWidth,
         const int screenHeight,
         SDL_Joystick *joystick) {
    Joystick leftJoystick, rightJoystick, triggers;
    initializeJoysticks(config, &leftJoystick, &rightJoystick, hasTriggers ? &triggers : nullptr);
    std::unordered_map<int, int> buttonMapping;
    for (const auto &configKey : config["button_mapping"].getMemberNames()) {
        int key = config["button_mapping"][configKey].asInt() - 1;
        buttonMapping[key] = BUTTON_NAME_TO_BUTTON_ID.at(configKey);
    }
    bool running = config["run_automatically"].asBool();
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
    float currentRadius = MAX_RADIUS_HIGH_PRECISION_OFF;

    Functions functions;
    const int center_x = screenWidth / 2;
    const int center_y = screenHeight / 2;

    auto left_card_pos = std::make_pair(720, center_y);
    auto right_card_pos = std::make_pair(1200, center_y);
    auto center_pos = std::make_pair(center_x, center_y);
    auto reroll_pos = std::make_pair(center_x, 825);

    const auto INPUT_TO_KEY_TAP = std::unordered_map<int, WORD>{{START, VK_ESCAPE}, {X, 'C'}, {Y, 'O'}};
    const auto INPUT_TO_KEY_HOLD = std::unordered_map<int, WORD>{
        {LEFT_JS_LEFT, 'A'},
        {LEFT_JS_RIGHT, 'D'},
        {LEFT_JS_UP, 'W'},
        {LEFT_JS_DOWN, 'S'},
        {B, VK_TAB},
        {R2, 'T'},
    };
    // TODO: use a "mouseTarget" reference instead, that can be dynamically applied res scaling to
    const auto INPUT_TO_MOUSE_MOVE = std::unordered_map<int, std::pair<int, int> *>{
        {PAD_LEFT, &left_card_pos},
        {PAD_RIGHT, &right_card_pos},
        {PAD_UP, &center_pos},
        {PAD_DOWN, &reroll_pos},
        {R3, &center_pos},
    };
    const auto INPUT_TO_MOUSE_CLICK = std::unordered_map<int, int>{{A, SDL_BUTTON_LEFT}};
    const auto RELEASE_TO_KEY_TAP = std::unordered_map<int, WORD>{{R1, 'R'}, {L1, 'E'}};
    const auto INPUT_TO_LOGIC_BEFORE = std::unordered_map<int, std::function<bool()>>{
        {R2, [&]() { functions.moveMouse(center_x, center_y); return true; }},
        {R3, [&]() { highPrecisionAlwaysOn = !highPrecisionAlwaysOn; return true; }},
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
                    functions.handleToMouseAbsoluteMove(input, JUST_PRESSED);
                }
                for (const auto &[input, _] : RELEASE_TO_KEY_TAP) {
                    functions.handleToKeyTap(input, JUST_RELEASED);
                }
                for (const auto &[input, _] : INPUT_TO_MOUSE_CLICK) {
                    functions.handleToClick(input, JUST_PRESSED);
                }

                if (rightJoystick.isXActive || rightJoystick.isYActive) {
                    if (highPrecisionAlwaysOn || highPrecision) {
                        if (std::chrono::steady_clock::now() - lastUpdateTime > std::chrono::milliseconds(100)) {
                            functions.moveMouseRelative(
                                round(rightJoystick.x * rightJoystick.sensitivity * 500),
                                round(rightJoystick.y * rightJoystick.sensitivity * 500));
                            lastUpdateTime = std::chrono::steady_clock::now();
                        }
                    } else {
                        functions.moveMouse(
                            round(rightJoystick.x * static_cast<float>(center_x) * currentRadius + center_x),
                            round(rightJoystick.y * static_cast<float>(center_y) * currentRadius + center_y));
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
