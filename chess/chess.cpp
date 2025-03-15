#include "chess.h"
#include "configParser.h"
#include "constants.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>
#include <windows.h>

namespace chess {
constexpr std::array<std::array<std::pair<int, int>, 8>, 8> BOARD_COORDINATES = {
    {{{{304, 929}, {413, 928}, {511, 928}, {624, 925}, {725, 924}, {829, 925}, {937, 926}, {1039, 928}}},
     {{{308, 819}, {412, 822}, {518, 820}, {623, 822}, {724, 824}, {830, 822}, {934, 823}, {1042, 823}}},
     {{{295, 712}, {411, 713}, {510, 709}, {620, 712}, {724, 703}, {830, 712}, {930, 709}, {1035, 709}}},
     {{{295, 607}, {409, 606}, {512, 610}, {619, 605}, {718, 607}, {829, 606}, {931, 603}, {1034, 605}}},
     {{{294, 499}, {405, 501}, {506, 499}, {618, 496}, {719, 498}, {828, 497}, {936, 497}, {1035, 497}}},
     {{{299, 391}, {409, 393}, {513, 389}, {615, 390}, {719, 391}, {829, 391}, {935, 399}, {1034, 394}}},
     {{{306, 295}, {409, 295}, {511, 295}, {616, 295}, {725, 295}, {826, 299}, {933, 296}, {1037, 296}}},
     {{{304, 190}, {412, 190}, {517, 189}, {615, 189}, {722, 187}, {830, 190}, {935, 193}, {1044, 192}}}}};

constexpr std::array<std::pair<int, int>, 2> RESIGN_YES_NO = {
    {{1176, 510}, {1120, 505}}};

constexpr std::array<std::pair<int, int>, 2> DRAW_YES_NO = {
    {{1182, 450}, {1118, 448}}};

void run(std::unordered_map<int, int> &buttonState,
         const Json::Value &config,
         const int screenWidth,
         const int screenHeight) {
    std::unordered_map<int, int> buttonMapping = configParser::readButtonMapping(config);
    bool running = configParser::readRunAutomatically(config);

    Functions functions;
    constexpr std::pair<int, int> PLAY_AGAIN = {1255, 539};
    constexpr std::pair<int, int> REMATCH = {1431, 536};
    constexpr std::pair<int, int> RESIGN = {1177, 570};
    constexpr std::pair<int, int> DRAW = {1163, 511};
    const auto TURBO_INPUTS = std::unordered_set<int>{PAD_LEFT, PAD_RIGHT, PAD_UP, PAD_DOWN};

    const double res_scaling_x = screenWidth / 1920;
    const double res_scaling_y = screenHeight / 1080;
    const auto now = std::chrono::steady_clock::now();

    State programState = {
        .boardRow = 0,
        .boardColumn = 0,
        .resignIndex = 0,
        .drawIndex = 0,
        .mode = Mode::BOARD,
        .mouse_target = {},
    };
    BufferState bufferState = {
        .last_pressed = now,
        .last_executed = now,
        .is_unleashed = false,
    };

    const auto INPUT_TO_MOUSE_MOVE = std::unordered_map<int, std::pair<int, int> *>{
        {PAD_LEFT, &programState.mouse_target},
        {PAD_RIGHT, &programState.mouse_target},
        {PAD_UP, &programState.mouse_target},
        {PAD_DOWN, &programState.mouse_target}};
    const auto INPUT_TO_BUTTON_CLICK = std::unordered_map<int, int>{{B, SDL_BUTTON_RIGHT}, {R1, SDL_BUTTON_LEFT}, {L1, SDL_BUTTON_LEFT}, {Y, SDL_BUTTON_LEFT}, {X, SDL_BUTTON_LEFT}};
    const auto INPUT_TO_BUTTON_TOGGLE = std::unordered_map<int, int>{{A, SDL_BUTTON_LEFT}};
    const auto RELEASE_TO_BUTTON_TOGGLE = std::unordered_map<int, int>{{A, SDL_BUTTON_LEFT}};
    const auto INPUT_TO_LOGIC_BEFORE = std::unordered_map<int, std::function<bool()>>{
        {L1, [&]() { functions.moveMouse(static_cast<int>(REMATCH.first * res_scaling_x), static_cast<int>(REMATCH.second * res_scaling_y)); return true; }},
        {R1, [&]() { functions.moveMouse(static_cast<int>(PLAY_AGAIN.first * res_scaling_x), static_cast<int>(PLAY_AGAIN.second * res_scaling_y)); return true; }},
        {X, [&]() { functions.moveMouse(static_cast<int>(DRAW.first * res_scaling_x), static_cast<int>(DRAW.second * res_scaling_y)); return true; }},
        {Y, [&]() { functions.moveMouse(static_cast<int>(RESIGN.first * res_scaling_x), static_cast<int>(RESIGN.second * res_scaling_y)); return true; }},
        {PAD_LEFT, [&]() { return updateAbstractState(PAD_LEFT, programState, bufferState, res_scaling_x, res_scaling_y, functions); }},
        {PAD_RIGHT, [&]() { return updateAbstractState(PAD_RIGHT, programState, bufferState, res_scaling_x, res_scaling_y, functions); }},
        {PAD_UP, [&]() { return updateAbstractState(PAD_UP, programState, bufferState, res_scaling_x, res_scaling_y, functions); }},
        {PAD_DOWN, [&]() { return updateAbstractState(PAD_DOWN, programState, bufferState, res_scaling_x, res_scaling_y, functions); }},
    };
    const auto INPUT_TO_LOGIC_AFTER = std::unordered_map<int, std::function<bool()>>{
        {L1, [&]() { auto [x, y] = BOARD_COORDINATES[programState.boardRow][programState.boardColumn]; functions.moveMouse(static_cast<int>(x * res_scaling_x), static_cast<int>(y * res_scaling_y)); return true; }},
        {R1, [&]() { auto [x, y] = BOARD_COORDINATES[programState.boardRow][programState.boardColumn]; functions.moveMouse(static_cast<int>(x * res_scaling_x), static_cast<int>(y * res_scaling_y)); return true; }},
        {X, [&]() { programState.mode = Mode::DRAW; auto [x, y] = DRAW_YES_NO[programState.drawIndex]; functions.moveMouse(static_cast<int>(x * res_scaling_x), static_cast<int>(y * res_scaling_y)); return true; }},
        {Y, [&]() { programState.mode = Mode::RESIGN; auto [x, y] = RESIGN_YES_NO[programState.resignIndex]; functions.moveMouse(static_cast<int>(x * res_scaling_x), static_cast<int>(y * res_scaling_y)); return true; }},
        {A, [&]() { if (programState.mode != Mode::BOARD) {programState.mode = Mode::BOARD; programState.drawIndex = 0; programState.resignIndex = 0;} return true; }},
    };

    functions.setMaps(&buttonState, &INPUT_TO_MOUSE_MOVE, nullptr, &INPUT_TO_BUTTON_CLICK, nullptr, &INPUT_TO_BUTTON_TOGGLE, &RELEASE_TO_BUTTON_TOGGLE, nullptr, nullptr, nullptr, &INPUT_TO_LOGIC_BEFORE, &INPUT_TO_LOGIC_AFTER, nullptr, nullptr);

    try {
        while (true) {
            auto loop_start_time = std::chrono::steady_clock::now();
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

                // action
                for (const auto &[input, _] : INPUT_TO_MOUSE_MOVE) {
                    if (TURBO_INPUTS.find(input) != TURBO_INPUTS.end()) {
                        functions.handleToMouseAbsoluteMove(input, PRESSED);
                    }
                    functions.handleToMouseAbsoluteMove(input, JUST_PRESSED);
                }
                for (const auto &[input, _] : INPUT_TO_BUTTON_CLICK) {
                    functions.handleToClick(input, JUST_PRESSED);
                }
                for (const auto &[input, _] : INPUT_TO_BUTTON_TOGGLE) {
                    functions.handleToButtonToggle(input, JUST_PRESSED);
                }
                for (const auto &[input, _] : RELEASE_TO_BUTTON_TOGGLE) {
                    functions.handleToButtonToggle(input, JUST_RELEASED);
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(std::max(
                10000 - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - loop_start_time).count(), 0LL)));
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

bool updateAbstractState(const int button, State &state, BufferState &bufferState, const double res_scaling_x, const double res_scaling_y, const Functions &functions) {
    if (!functions.isBufferFree(200, 50, button, bufferState)) {
        return false;
    }

    std::pair<int, int> coordinates;
    if (state.mode == Mode::BOARD) {
        if (button == PAD_UP) {
            state.boardRow = (state.boardRow + 1) % 8;
        } else if (button == PAD_DOWN) {
            state.boardRow = (state.boardRow + 7) % 8;
        } else if (button == PAD_LEFT) {
            state.boardColumn = (state.boardColumn + 7) % 8;
        } else if (button == PAD_RIGHT) {
            state.boardColumn = (state.boardColumn + 1) % 8;
        }
        coordinates = BOARD_COORDINATES[state.boardRow][state.boardColumn];
    } else if (state.mode == Mode::RESIGN) {
        if (button == PAD_LEFT || button == PAD_RIGHT) {
            state.resignIndex = 1 - state.resignIndex;
            coordinates = RESIGN_YES_NO[state.resignIndex];
        } else {
            return false;
        }
    } else if (state.mode == Mode::DRAW) {
        if (button == PAD_LEFT || button == PAD_RIGHT) {
            state.drawIndex = 1 - state.drawIndex;
            coordinates = DRAW_YES_NO[state.drawIndex];
        } else {
            return false;
        }
    }
    state.mouse_target = {coordinates.first * res_scaling_x, coordinates.second * res_scaling_y};
    return true;
}
} // namespace chess
