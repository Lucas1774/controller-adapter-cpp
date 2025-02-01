#include "tft.h"
#include "joystick.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>
#include <windows.h>

namespace tft {
const std::vector<std::pair<int, int>> MOVE_COORDINATES = {
    {483, 656}, {473, 438}, {542, 175}, {927, 180}, {1286, 198}, {1418, 476}, {1447, 671}, {956, 679}};

const std::vector<std::vector<std::pair<int, int>>> BOARD_COORDINATES = {
    {{427, 756}, {544, 751}, {659, 757}, {776, 754}, {895, 756}, {1011, 754}, {1125, 754}, {1246, 752}, {1356, 752}},
    {{583, 632}, {699, 630}, {839, 634}, {960, 637}, {1096, 637}, {1214, 642}, {1340, 644}},
    {{535, 555}, {663, 559}, {783, 559}, {904, 565}, {1027, 564}, {1148, 561}, {1265, 565}},
    {{611, 482}, {727, 487}, {845, 489}, {961, 485}, {1081, 484}, {1189, 485}, {1314, 489}},
    {{567, 423}, {680, 426}, {794, 427}, {904, 427}, {1023, 429}, {1133, 422}, {1246, 420}}};

const std::vector<std::vector<std::pair<int, int>>> ITEM_COORDINATES = {
    {{30, 298}, {30, 349}, {30, 399}, {30, 452}, {30, 502}, {30, 549}, {30, 601}, {30, 654}, {30, 706}, {30, 754}},
    {{80, 298}, {80, 349}, {80, 399}, {80, 452}, {80, 502}, {80, 549}, {80, 601}, {80, 654}, {80, 706}, {80, 754}}};

const std::vector<std::pair<int, int>> SHOP_COORDINATES = {
    {503, 982}, {714, 982}, {923, 985}, {1151, 984}, {1348, 987}};

const std::vector<std::vector<std::pair<int, int>>> CARD_COORDINATES = {
    {{553, 580}, {963, 580}, {1380, 583}},
    {{552, 865}, {959, 866}, {1365, 865}}};

const std::vector<std::pair<int, int>> LOCK_COORDINATES = {{1450, 905}, {1323, 948}, {1327, 1027}};

constexpr int LOCK_ADJACENCY_MATRIX[3][3] = {
    {NONE, PAD_DOWN, PAD_UP}, // lock
    {PAD_UP, NONE, PAD_DOWN}, // 1
    {PAD_DOWN, PAD_UP, NONE}, // 2
};

void run(std::unordered_map<int, int> &buttonState,
         const bool &hasTriggers,
         const Json::Value &config,
         const int screenWidth,
         const int screenHeight,
         SDL_Joystick *joystick) {
    Joystick leftJoystick, rightJoystick, triggers;
    initializeJoysticks(config, &leftJoystick, &rightJoystick, hasTriggers ? &triggers : nullptr);
    std::unordered_map<int, int> buttonMapping;
    for (const std::string &configKey : config["button_mapping"].getMemberNames()) {
        int key = config["button_mapping"][configKey].asInt() - 1;
        buttonMapping[key] = BUTTON_NAME_TO_BUTTON_ID.at(configKey);
    }
    bool running = config["run_automatically"].asBool();

    Functions functions;
    const double res_scaling_x = screenWidth / 1920;
    const double res_scaling_y = screenHeight / 1080;
    const auto now = std::chrono::steady_clock::now();

    State state = {
        .boardRow = 2,
        .boardColumn = 3,
        .itemColumn = 0,
        .itemRow = 0,
        .shopIndex = 2,
        .cardRow = 0,
        .cardColumn = 1,
        .lockIndex = 0,
        .mode = MouseMovementWithPadMode::BOARD,
        .previous_mode = MouseMovementWithPadMode::BOARD,
        .mouse_target = {},
    };
    BufferState buffer_state = {
        .last_pressed = now,
        .last_executed = now,
        .is_unleashed = false};

    const auto TURBO_INPUTS = std::unordered_set<int>{PAD_LEFT, PAD_RIGHT, PAD_UP, PAD_DOWN};
    const auto INPUT_TO_KEY_TAP = std::unordered_map<int, WORD>{{B, 'E'}, {X, 'F'}, {Y, 'D'}, {R2, 'R'}, {L2, 'Q'}, {START, 'W'}};
    const auto INPUT_TO_MOUSE_CLICK = std::unordered_map<int, int>{{SELECT, SDL_BUTTON_RIGHT}};
    const auto INPUT_TO_MOUSE_MOVE = std::unordered_map<int, std::pair<int, int> *>{
        {PAD_LEFT, &state.mouse_target},
        {PAD_RIGHT, &state.mouse_target},
        {PAD_UP, &state.mouse_target},
        {PAD_DOWN, &state.mouse_target},
        {R1, &state.mouse_target},
        {L1, &state.mouse_target},
        {R3, &state.mouse_target},
        {L3, &state.mouse_target},
    };
    const auto RELEASE_TO_MOUSE_MOVE = std::unordered_map<int, std::pair<int, int> *>{
        {R1, &state.mouse_target},
        {L1, &state.mouse_target},
    };
    const auto INPUT_TO_LOGIC_BEFORE = std::unordered_map<int, std::function<bool()>>{
        {PAD_LEFT, [&]() { return updateAbstractState(PAD_LEFT, state, buffer_state, res_scaling_x, res_scaling_y, functions); }},
        {PAD_RIGHT, [&]() { return updateAbstractState(PAD_RIGHT, state, buffer_state, res_scaling_x, res_scaling_y, functions); }},
        {PAD_UP, [&]() { return updateAbstractState(PAD_UP, state, buffer_state, res_scaling_x, res_scaling_y, functions); }},
        {PAD_DOWN, [&]() { return updateAbstractState(PAD_DOWN, state, buffer_state, res_scaling_x, res_scaling_y, functions); }}};

    functions.setMaps(&buttonState, &INPUT_TO_MOUSE_MOVE, &RELEASE_TO_MOUSE_MOVE, &INPUT_TO_MOUSE_CLICK, nullptr, nullptr, nullptr, &INPUT_TO_KEY_TAP, nullptr, nullptr, &INPUT_TO_LOGIC_BEFORE, nullptr, nullptr, nullptr);

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
                // the code below is kind of horrible. It is what it is.
                // for item and board mode toggling, we remember positions to make it easier to build full items
                if (buttonState[L1] == JUST_PRESSED) {
                    state.mode = MouseMovementWithPadMode::ITEMS;
                    auto [x, y] = ITEM_COORDINATES[state.itemColumn][state.itemRow];
                    state.mouse_target = {x * res_scaling_x, y * res_scaling_y};
                } else if (buttonState[L1] == JUST_RELEASED) {
                    state.mode = MouseMovementWithPadMode::BOARD;
                    auto [x, y] = BOARD_COORDINATES[state.boardRow][state.boardColumn];
                    state.mouse_target = {x * res_scaling_x, y * res_scaling_y};
                    // for shop and board mode toggling, we always go to bench and to middle card, that is also the "show own board" button location
                    // we don't necessarily go back to board mode. If card mode was set as "previous", we go there.
                    // the idea is to easily show the board when going to pick a card, without leaving card selecting mode
                } else if (buttonState[R1] == JUST_PRESSED) {
                    state.mode = MouseMovementWithPadMode::SHOP;
                    state.shopIndex = 2;
                    auto [x, y] = SHOP_COORDINATES[state.shopIndex];
                    state.mouse_target = {x * res_scaling_x, y * res_scaling_y};
                } else if (buttonState[R1] == JUST_RELEASED) {
                    state.mode = state.previous_mode;
                    if (state.mode == MouseMovementWithPadMode::BOARD) {
                        state.boardRow = 0;
                        state.boardColumn = 0;
                        auto [x, y] = BOARD_COORDINATES[0][0];
                        state.mouse_target = {x * res_scaling_x, y * res_scaling_y};
                    } else if (state.mode == MouseMovementWithPadMode::CARDS) {
                        auto [x, y] = CARD_COORDINATES[state.cardRow][state.cardColumn];
                        state.mouse_target = {x * res_scaling_x, y * res_scaling_y};
                    }
                    // for card and board mode toggling, we also go to the bench when going to board mode, and to the middle when going to card mode, because why not
                    // we save the state in previous state in case we want to go to item state after.
                } else if (buttonState[R3] == JUST_PRESSED) {
                    if (state.mode == MouseMovementWithPadMode::CARDS) {
                        state.mode = MouseMovementWithPadMode::BOARD;
                        state.previous_mode = MouseMovementWithPadMode::BOARD;
                        state.boardRow = 0;
                        state.boardColumn = 0;
                        auto [x, y] = BOARD_COORDINATES[0][0];
                        state.mouse_target = {x * res_scaling_x, y * res_scaling_y};
                    } else {
                        state.mode = MouseMovementWithPadMode::CARDS;
                        state.previous_mode = MouseMovementWithPadMode::CARDS;
                        state.cardRow = 0;
                        state.cardColumn = 1;
                        auto [x, y] = CARD_COORDINATES[state.cardRow][state.cardColumn];
                        state.mouse_target = {x * res_scaling_x, y * res_scaling_y};
                    }
                    // for lock and board mode toggling, also to bench for convenience, and also always to lock
                } else if (buttonState[L3] == JUST_PRESSED) {
                    if (state.mode == MouseMovementWithPadMode::LOCK) {
                        state.mode = MouseMovementWithPadMode::BOARD;
                        state.boardRow = 0;
                        state.boardColumn = 0;
                        auto [x, y] = BOARD_COORDINATES[0][0];
                        state.mouse_target = {x * res_scaling_x, y * res_scaling_y};
                    } else {
                        state.mode = MouseMovementWithPadMode::LOCK;
                        state.lockIndex = 0;
                        auto [x, y] = LOCK_COORDINATES[state.lockIndex];
                        state.mouse_target = {x * res_scaling_x, y * res_scaling_y};
                    }
                }

                // action
                for (const auto &[input, _] : INPUT_TO_KEY_TAP) {
                    functions.handleToKeyTap(input, JUST_PRESSED);
                }
                for (const auto &[input, _] : INPUT_TO_MOUSE_MOVE) {
                    if (TURBO_INPUTS.find(input) != TURBO_INPUTS.end()) {
                        functions.handleToMouseAbsoluteMove(input, PRESSED);
                    }
                    functions.handleToMouseAbsoluteMove(input, JUST_PRESSED);
                }
                for (const auto &[input, _] : RELEASE_TO_MOUSE_MOVE) {
                    functions.handleToMouseAbsoluteMove(input, JUST_RELEASED);
                }
                for (const auto &[input, _] : INPUT_TO_MOUSE_CLICK) {
                    functions.handleToClick(input, JUST_PRESSED);
                }
                if (state.mode != MouseMovementWithPadMode::ITEMS) {
                    functions.handleToClick(A, SDL_BUTTON_LEFT, JUST_PRESSED);
                } else {
                    functions.handleToButtonToggle(A, SDL_BUTTON_LEFT, JUST_PRESSED);
                }

                if (rightJoystick.isXActive || rightJoystick.isYActive) {
                    if (std::chrono::steady_clock::now() - lastUpdateTime > std::chrono::milliseconds(100)) {
                        state.mode = MouseMovementWithPadMode::FREE;
                        functions.moveMouseRelative(
                            static_cast<int>(round(rightJoystick.x * rightJoystick.sensitivity * 500 * res_scaling_x)),
                            static_cast<int>(round(rightJoystick.y * rightJoystick.sensitivity * 500 * res_scaling_y)));
                        lastUpdateTime = std::chrono::steady_clock::now();
                    }
                } else if (leftJoystick.isXActive || leftJoystick.isYActive) { // leftJoystick isActive breaks the program semantics:
                                                                               // The joystick has been "digitalized", however, the param is at hand and improves performance, so might as well use it.
                    updateAbstractState(state, res_scaling_x, res_scaling_y, functions);
                    functions.moveMouse(state.mouse_target.first, state.mouse_target.second);
                    if (std::chrono::steady_clock::now() - lastUpdateTime > std::chrono::milliseconds(100)) {
                        functions.click(SDL_BUTTON_RIGHT);
                        lastUpdateTime = std::chrono::steady_clock::now();
                    }
                }
            }

            std::this_thread::sleep_for(
                std::chrono::microseconds(std::max(
                    10000 - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - loopStartTime).count(), 0LL)));
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

bool updateAbstractState(const int button, State &state, BufferState &bufferState, const double res_scaling_x, const double res_scaling_y, const Functions &functions) {
    if (!functions.isBufferFree(200, 50, button, bufferState)) {
        return false;
    }

    if (state.mode == MouseMovementWithPadMode::FREE) {
        state.mode = MouseMovementWithPadMode::BOARD;
        state.boardRow = 0;
        state.boardColumn = 0;
        auto [x, y] = BOARD_COORDINATES[0][0];
        state.mouse_target = {x * res_scaling_x, y * res_scaling_y};
        return true;
    }
    std::pair<int, int> coordinates;
    if (state.mode == MouseMovementWithPadMode::BOARD) {
        if (button == PAD_UP) {
            state.boardRow = (state.boardRow + 1) % 5;
            if (state.boardRow != 0) {
                state.boardColumn = state.boardColumn > 6 ? 6 : state.boardColumn;
            }
        } else if (button == PAD_DOWN) {
            state.boardRow = (state.boardRow + 4) % 5;
            if (state.boardRow != 0) {
                state.boardColumn = state.boardColumn > 6 ? 6 : state.boardColumn;
            }
        } else if (button == PAD_LEFT) {
            if (state.boardRow != 0) {
                state.boardColumn = (state.boardColumn + 6) % 7;
            } else {
                state.boardColumn = (state.boardColumn + 8) % 9;
            }
        } else if (button == PAD_RIGHT) {
            if (state.boardRow != 0) {
                state.boardColumn = (state.boardColumn + 1) % 7;
            } else {
                state.boardColumn = (state.boardColumn + 1) % 9;
            }
        }
        coordinates = BOARD_COORDINATES[state.boardRow][state.boardColumn];
    } else if (state.mode == MouseMovementWithPadMode::ITEMS) {
        if (button == PAD_UP) {
            state.itemRow = (state.itemRow + 9) % 10;
        } else if (button == PAD_DOWN) {
            state.itemRow = (state.itemRow + 1) % 10;
        } else {
            state.itemColumn = (state.itemColumn + 1) % 2;
        }
        coordinates = ITEM_COORDINATES[state.itemColumn][state.itemRow];
    } else if (state.mode == MouseMovementWithPadMode::SHOP) {
        if (button == PAD_LEFT) {
            state.shopIndex = (state.shopIndex + 4) % 5;
        } else if (button == PAD_RIGHT) {
            state.shopIndex = (state.shopIndex + 1) % 5;
        }
        coordinates = SHOP_COORDINATES[state.shopIndex];
    } else if (state.mode == MouseMovementWithPadMode::CARDS) {
        if (button == PAD_UP || button == PAD_DOWN) {
            state.cardRow = (state.cardRow + 1) % 2;
        } else if (button == PAD_RIGHT) {
            state.cardColumn = (state.cardColumn + 1) % 3;
        } else if (button == PAD_LEFT) {
            state.cardColumn = (state.cardColumn + 2) % 3;
        }
        coordinates = CARD_COORDINATES[state.cardRow][state.cardColumn];
    } else if (state.mode == MouseMovementWithPadMode::LOCK) {
        for (int i = 0; i < 7; i++) {
            if (LOCK_ADJACENCY_MATRIX[state.lockIndex][i] == button) {
                state.lockIndex = i;
                break;
            }
        }
        coordinates = LOCK_COORDINATES[state.lockIndex];
    }
    state.mouse_target = {coordinates.first * res_scaling_x, coordinates.second * res_scaling_y};
    return true;
}

void updateAbstractState(State &state, const double res_scaling_x, const double res_scaling_y, const Functions &functions) {
    auto [x, y] = MOVE_COORDINATES[functions.generateAxisTargetWithBitMask(LEFT_JS)];
    state.mouse_target = {x * res_scaling_x, y * res_scaling_y};
}
} // namespace tft
