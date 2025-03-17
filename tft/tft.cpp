#include "tft.h"
#include "configParser.h"
#include "funcs.h"
#include "joystick.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <windows.h>

namespace tft {

enum class MouseMovementWithPadMode {
    BOARD = 0,
    ITEMS,
    SHOP,
    CARDS,
    LOCK,
    FREE,
};

struct State {
    int boardRow;
    int boardColumn;
    int itemColumn;
    int itemRow;
    int shopRow;
    int shopColumn;
    int cardRow;
    int cardColumn;
    int lockIndex;
    MouseMovementWithPadMode mode;
    MouseMovementWithPadMode previous_mode;
    std::pair<int, int> mouseTarget;
};

static constexpr std::array<std::pair<int, int>, 8> MOVE_COORDINATES = {
    {{483, 656}, {473, 438}, {542, 175}, {927, 180}, {1286, 198}, {1418, 476}, {1447, 671}, {956, 679}}};
static constexpr std::array<std::array<std::pair<int, int>, 9>, 5> BOARD_COORDINATES = {
    {{{{427, 756}, {544, 751}, {659, 757}, {776, 754}, {895, 756}, {1011, 754}, {1125, 754}, {1246, 752}, {1356, 752}}},
     {{{583, 632}, {699, 630}, {839, 634}, {960, 637}, {1096, 637}, {1214, 642}, {1340, 644}}},
     {{{535, 555}, {663, 559}, {783, 559}, {904, 565}, {1027, 564}, {1148, 561}, {1265, 565}}},
     {{{611, 482}, {727, 487}, {845, 489}, {961, 485}, {1081, 484}, {1189, 485}, {1314, 489}}},
     {{{567, 423}, {680, 426}, {794, 427}, {904, 427}, {1023, 429}, {1133, 422}, {1246, 420}}}}};
static constexpr std::array<std::array<std::pair<int, int>, 2>, 10> ITEM_COORDINATES = {
    {{{{30, 298}, {80, 298}}},
     {{{30, 349}, {80, 349}}},
     {{{30, 399}, {80, 399}}},
     {{{30, 452}, {80, 452}}},
     {{{30, 502}, {80, 502}}},
     {{{30, 549}, {80, 549}}},
     {{{30, 601}, {80, 601}}},
     {{{30, 654}, {80, 654}}},
     {{{30, 706}, {80, 706}}},
     {{{30, 754}, {80, 754}}}}};
static constexpr std::array<std::array<std::pair<int, int>, 5>, 1> SHOP_COORDINATES = {
    {{{{503, 982}, {714, 982}, {923, 985}, {1151, 984}, {1348, 987}}}}};
static constexpr std::array<std::array<std::pair<int, int>, 3>, 2> CARD_COORDINATES = {
    {{{{553, 580}, {963, 580}, {1380, 583}}},
     {{{552, 865}, {959, 866}, {1365, 865}}}}};
static constexpr std::array<std::pair<int, int>, 3> LOCK_COORDINATES = {
    {{1450, 905}, {1323, 948}, {1327, 1027}}};
static constexpr std::array<std::array<int, 3>, 3> LOCK_ADJACENCY_MATRIX = {
    {{NONE, PAD_DOWN, PAD_UP},   // lock
     {PAD_UP, NONE, PAD_DOWN},   // 1
     {PAD_DOWN, PAD_UP, NONE}}}; // 2

static bool updateAbstractState(const int button, State &state, BufferState &bufferState, const Functions &functions) {
    if (!functions.isBufferFree(200, 50, button, bufferState)) {
        return false;
    }

    if (state.mode == MouseMovementWithPadMode::FREE) {
        state.mode = MouseMovementWithPadMode::BOARD;
        state.boardRow = 0;
        state.boardColumn = 0;
        state.mouseTarget = BOARD_COORDINATES[0][0];
        return true;
    }

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
        state.mouseTarget = BOARD_COORDINATES[state.boardRow][state.boardColumn];
        return true;
    }

    static const std::map<MouseMovementWithPadMode, std::function<bool()>> modeToFunction = {
        {MouseMovementWithPadMode::ITEMS, [&functions, &state, &button]() {
             return functions.computeGridBasedTarget(ITEM_COORDINATES, state.mouseTarget, state.itemRow, state.itemColumn, button);
         }},
        {MouseMovementWithPadMode::SHOP, [&functions, &state, &button]() {
             return functions.computeGridBasedTarget(SHOP_COORDINATES, state.mouseTarget, state.shopRow, state.shopColumn, button);
         }},
        {MouseMovementWithPadMode::CARDS, [&functions, &state, &button]() {
             return functions.computeGridBasedTarget(CARD_COORDINATES, state.mouseTarget, state.cardRow, state.cardColumn, button);
         }},
        {MouseMovementWithPadMode::LOCK, [&functions, &state, &button]() {
             return functions.computeAdjacencyMatrixBasedTarget(LOCK_ADJACENCY_MATRIX, LOCK_COORDINATES, state.mouseTarget, state.lockIndex, button);
         }}};

    if (auto it = modeToFunction.find(state.mode); it != modeToFunction.end()) {
        return it->second();
    }
    return false;
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
        .boardRow = 2,
        .boardColumn = 3,
        .itemColumn = 0,
        .itemRow = 0,
        .shopRow = 0,
        .shopColumn = 2,
        .cardRow = 0,
        .cardColumn = 1,
        .lockIndex = 0,
        .mode = MouseMovementWithPadMode::BOARD,
        .previous_mode = MouseMovementWithPadMode::BOARD,
        .mouseTarget = {}};
    BufferState buffer_state = {
        .lastPressed = now,
        .lastExecuted = now,
        .isUnleashed = false};

    const auto TURBO_INPUTS = std::unordered_set<int>{PAD_LEFT, PAD_RIGHT, PAD_UP, PAD_DOWN};
    const auto INPUT_TO_KEY_TAP = std::unordered_map<int, std::function<WORD()>>{
        {B, [] { return 'E'; }},
        {X, [] { return 'F'; }},
        {Y, [] { return 'D'; }},
        {R2, [] { return 'R'; }},
        {L2, [] { return 'Q'; }},
        {START, [] { return 'W'; }}};
    const auto INPUT_TO_MOUSE_CLICK = std::unordered_map<int, std::function<int()>>{
        {SELECT, [] { return SDL_BUTTON_RIGHT; }}};
    const auto INPUT_TO_MOUSE_MOVE = std::unordered_map<int, std::function<std::pair<int, int>()>>{
        {PAD_LEFT, [&state] { return state.mouseTarget; }},
        {PAD_RIGHT, [&state] { return state.mouseTarget; }},
        {PAD_UP, [&state] { return state.mouseTarget; }},
        {PAD_DOWN, [&state] { return state.mouseTarget; }},
        {R1, [&state] { return state.mouseTarget; }},
        {L1, [&state] { return state.mouseTarget; }},
        {R3, [&state] { return state.mouseTarget; }},
        {L3, [&state] { return state.mouseTarget; }}};
    const auto RELEASE_TO_MOUSE_MOVE = std::unordered_map<int, std::function<std::pair<int, int>()>>{
        {R1, [&state] { return state.mouseTarget; }},
        {L1, [&state] { return state.mouseTarget; }}};
    const auto INPUT_TO_LOGIC_BEFORE = std::unordered_map<int, std::function<bool()>>{
        {PAD_LEFT, [&functions, &state, &buffer_state]() { return updateAbstractState(PAD_LEFT, state, buffer_state, functions); }},
        {PAD_RIGHT, [&functions, &state, &buffer_state]() { return updateAbstractState(PAD_RIGHT, state, buffer_state, functions); }},
        {PAD_UP, [&functions, &state, &buffer_state]() { return updateAbstractState(PAD_UP, state, buffer_state, functions); }},
        {PAD_DOWN, [&functions, &state, &buffer_state]() { return updateAbstractState(PAD_DOWN, state, buffer_state, functions); }}};

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
                    state.mouseTarget = ITEM_COORDINATES[state.itemColumn][state.itemRow];
                } else if (buttonState[L1] == JUST_RELEASED) {
                    state.mode = MouseMovementWithPadMode::BOARD;
                    state.mouseTarget = BOARD_COORDINATES[state.boardRow][state.boardColumn];
                    // for shop and board mode toggling, we always go to bench and to middle card, that is also the "show own board" button location
                    // we don't necessarily go back to board mode. If card mode was set as "previous", we go there.
                    // the idea is to easily show the board when going to pick a card, without leaving card selecting mode
                } else if (buttonState[R1] == JUST_PRESSED) {
                    state.mode = MouseMovementWithPadMode::SHOP;
                    state.shopColumn = 2;
                    state.mouseTarget = SHOP_COORDINATES[0][state.shopColumn];
                } else if (buttonState[R1] == JUST_RELEASED) {
                    state.mode = state.previous_mode;
                    if (state.mode == MouseMovementWithPadMode::BOARD) {
                        state.boardRow = 0;
                        state.boardColumn = 0;
                        state.mouseTarget = BOARD_COORDINATES[0][0];
                    } else if (state.mode == MouseMovementWithPadMode::CARDS) {
                        state.mouseTarget = CARD_COORDINATES[state.cardRow][state.cardColumn];
                    }
                    // for card and board mode toggling, we also go to the bench when going to board mode, and to the middle when going to card mode, because why not
                    // we save the state in previous state in case we want to go to item state after.
                } else if (buttonState[R3] == JUST_PRESSED) {
                    if (state.mode == MouseMovementWithPadMode::CARDS) {
                        state.mode = MouseMovementWithPadMode::BOARD;
                        state.previous_mode = MouseMovementWithPadMode::BOARD;
                        state.boardRow = 0;
                        state.boardColumn = 0;
                        state.mouseTarget = BOARD_COORDINATES[0][0];
                    } else {
                        state.mode = MouseMovementWithPadMode::CARDS;
                        state.previous_mode = MouseMovementWithPadMode::CARDS;
                        state.cardRow = 0;
                        state.cardColumn = 1;
                        state.mouseTarget = CARD_COORDINATES[state.cardRow][state.cardColumn];
                    }
                    // for lock and board mode toggling, also to bench for convenience, and also always to lock
                } else if (buttonState[L3] == JUST_PRESSED) {
                    if (state.mode == MouseMovementWithPadMode::LOCK) {
                        state.mode = MouseMovementWithPadMode::BOARD;
                        state.boardRow = 0;
                        state.boardColumn = 0;
                        state.mouseTarget = BOARD_COORDINATES[0][0];
                    } else {
                        state.mode = MouseMovementWithPadMode::LOCK;
                        state.lockIndex = 0;
                        state.mouseTarget = LOCK_COORDINATES[state.lockIndex];
                    }
                }

                // action
                for (const auto &[input, _] : INPUT_TO_KEY_TAP) {
                    functions.handleToKeyTap(input, JUST_PRESSED);
                }
                for (const auto &[input, _] : INPUT_TO_MOUSE_MOVE) {
                    if (TURBO_INPUTS.find(input) != TURBO_INPUTS.end()) {
                        functions.handleToMouseAbsoluteMove(input, PRESSED, resScalingX, resScalingY);
                    }
                    functions.handleToMouseAbsoluteMove(input, JUST_PRESSED, resScalingX, resScalingY);
                }
                for (const auto &[input, _] : RELEASE_TO_MOUSE_MOVE) {
                    functions.handleToMouseAbsoluteMove(input, JUST_RELEASED, resScalingX, resScalingY);
                }
                for (const auto &[input, _] : INPUT_TO_MOUSE_CLICK) {
                    functions.handleToClick(input, JUST_PRESSED);
                }
                if (state.mode != MouseMovementWithPadMode::ITEMS) {
                    functions.handleToClick(A, JUST_PRESSED, SDL_BUTTON_LEFT);
                } else {
                    functions.handleToButtonToggle(A, JUST_PRESSED, SDL_BUTTON_LEFT);
                }

                if (rightJoystick.isXActive || rightJoystick.isYActive) {
                    if (std::chrono::steady_clock::now() - lastUpdateTime > std::chrono::milliseconds(16)) {
                        state.mode = MouseMovementWithPadMode::FREE;
                        functions.moveMouseRelative(
                            static_cast<int>(round(rightJoystick.x * rightJoystick.sensitivity * 100)),
                            static_cast<int>(round(rightJoystick.y * rightJoystick.sensitivity * 100)),
                            resScalingX, resScalingY);
                        lastUpdateTime = std::chrono::steady_clock::now();
                    }
                } else if (leftJoystick.isXActive || leftJoystick.isYActive) { // leftJoystick isActive breaks the program semantics:
                                                                               // The joystick has been "digitalized", however, the param is at hand and improves performance, so might as well use it.
                    state.mouseTarget = MOVE_COORDINATES[functions.generateAxisTargetWithBitMask(LEFT_JS)];
                    functions.moveMouse(state.mouseTarget.first, state.mouseTarget.second, resScalingX, resScalingY);
                    if (std::chrono::steady_clock::now() - lastUpdateTime > std::chrono::milliseconds(100)) {
                        functions.click(SDL_BUTTON_RIGHT);
                        lastUpdateTime = std::chrono::steady_clock::now();
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

} // namespace tft
