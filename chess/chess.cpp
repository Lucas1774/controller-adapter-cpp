#include "chess.h"
#include "configParser.h"
#include "funcs.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <windows.h>

namespace chess {

enum class Mode {
    BOARD,
    RESIGN,
    DRAW,
};

struct State {
    int boardRow;
    int boardColumn;
    int resignRow;
    int resignColumn;
    int drawRow;
    int drawColumn;
    Mode mode;
    std::pair<int, int> mouse_target;
};

static constexpr std::array<std::array<std::pair<int, int>, 8>, 8> BOARD_COORDINATES = {
    {{{{304, 190}, {412, 190}, {517, 189}, {615, 189}, {722, 187}, {830, 190}, {935, 193}, {1044, 192}}},
     {{{306, 295}, {409, 295}, {511, 295}, {616, 295}, {725, 295}, {826, 299}, {933, 296}, {1037, 296}}},
     {{{299, 391}, {409, 393}, {513, 389}, {615, 390}, {719, 391}, {829, 391}, {935, 399}, {1034, 394}}},
     {{{294, 499}, {405, 501}, {506, 499}, {618, 496}, {719, 498}, {828, 497}, {936, 497}, {1035, 497}}},
     {{{295, 607}, {409, 606}, {512, 610}, {619, 605}, {718, 607}, {829, 606}, {931, 603}, {1034, 605}}},
     {{{295, 712}, {411, 713}, {510, 709}, {620, 712}, {724, 703}, {830, 712}, {930, 709}, {1035, 709}}},
     {{{308, 819}, {412, 822}, {518, 820}, {623, 822}, {724, 824}, {830, 822}, {934, 823}, {1042, 823}}},
     {{{304, 929}, {413, 928}, {511, 928}, {624, 925}, {725, 924}, {829, 925}, {937, 926}, {1039, 928}}}}};

static constexpr std::array<std::array<std::pair<int, int>, 2>, 1> RESIGN_YES_NO = {
    {{{{1176, 510}, {1120, 505}}}}};

static constexpr std::array<std::array<std::pair<int, int>, 2>, 1> DRAW_YES_NO = {
    {{{{1182, 450}, {1118, 448}}}}};

static constexpr std::pair<int, int> PLAY_AGAIN = {1255, 539};
static constexpr std::pair<int, int> REMATCH = {1431, 536};
static constexpr std::pair<int, int> RESIGN = {1177, 570};
static constexpr std::pair<int, int> DRAW = {1163, 511};

static bool updateAbstractState(const int button, State &state, BufferState &bufferState, const Functions &functions) {
    if (!functions.isBufferFree(200, 50, button, bufferState)) {
        return false;
    }

    static const std::map<Mode, std::function<bool()>> modeToFunction = {
        {Mode::BOARD, [&functions, &state, &button]() {
             return functions.computeGridBasedMouseTarget(BOARD_COORDINATES, state.mouse_target, state.boardRow, state.boardColumn, button);
         }},
        {Mode::RESIGN, [&functions, &state, &button]() {
             return functions.computeGridBasedMouseTarget(RESIGN_YES_NO, state.mouse_target, state.resignRow, state.resignColumn, button);
         }},
        {Mode::DRAW, [&functions, &state, &button]() {
             return functions.computeGridBasedMouseTarget(DRAW_YES_NO, state.mouse_target, state.drawRow, state.drawColumn, button);
         }}};

    if (auto it = modeToFunction.find(state.mode); it != modeToFunction.end()) {
        return it->second();
    }
    return false;
}

void run(std::unordered_map<int, int> &buttonState,
         const Json::Value &config,
         const int screenWidth,
         const int screenHeight) {
    std::unordered_map<int, int> buttonMapping = configParser::readButtonMapping(config);
    bool running = configParser::readRunAutomatically(config);

    Functions functions;
    const double resScalingX = screenWidth / 1920.0;
    const double resScalingY = screenHeight / 1080.0;
    const auto now = std::chrono::steady_clock::now();

    State programState = {
        .boardRow = 0,
        .boardColumn = 0,
        .resignRow = 0,
        .resignColumn = 0,
        .drawRow = 0,
        .drawColumn = 0,
        .mode = Mode::BOARD,
        .mouse_target = {}};
    BufferState bufferState = {
        .last_pressed = now,
        .last_executed = now,
        .is_unleashed = false};

    const auto TURBO_INPUTS = std::unordered_set<int>{PAD_LEFT, PAD_RIGHT, PAD_UP, PAD_DOWN};
    const auto INPUT_TO_MOUSE_MOVE = std::unordered_map<int, std::function<std::pair<int, int>()>>{
        {PAD_LEFT, [&programState]() { return programState.mouse_target; }},
        {PAD_RIGHT, [&programState]() { return programState.mouse_target; }},
        {PAD_UP, [&programState]() { return programState.mouse_target; }},
        {PAD_DOWN, [&programState]() { return programState.mouse_target; }}};
    const auto INPUT_TO_BUTTON_CLICK = std::unordered_map<int, std::function<int()>>{
        {B, []() { return SDL_BUTTON_RIGHT; }},
        {R1, []() { return SDL_BUTTON_LEFT; }},
        {L1, []() { return SDL_BUTTON_LEFT; }},
        {Y, []() { return SDL_BUTTON_LEFT; }},
        {X, []() { return SDL_BUTTON_LEFT; }}};
    const auto INPUT_TO_BUTTON_TOGGLE = std::unordered_map<int, std::function<int()>>{{A, []() { return SDL_BUTTON_LEFT; }}};
    const auto RELEASE_TO_BUTTON_TOGGLE = std::unordered_map<int, std::function<int()>>{{A, []() { return SDL_BUTTON_LEFT; }}};
    const auto INPUT_TO_LOGIC_BEFORE = std::unordered_map<int, std::function<bool()>>{
        {L1, [&functions, resScalingX, resScalingY]() { functions.moveMouse(REMATCH.first,REMATCH.second, resScalingX, resScalingY); return true; }},
        {R1, [&functions, resScalingX, resScalingY]() { functions.moveMouse(PLAY_AGAIN.first,PLAY_AGAIN.second, resScalingX, resScalingY); return true; }},
        {X, [&functions, resScalingX, resScalingY]() { functions.moveMouse(DRAW.first,DRAW.second, resScalingX, resScalingY); return true; }},
        {Y, [&functions, resScalingX, resScalingY]() { functions.moveMouse(RESIGN.first,RESIGN.second, resScalingX, resScalingY); return true; }},
        {PAD_LEFT, [&functions, &programState, &bufferState]() {
             return updateAbstractState(PAD_LEFT, programState, bufferState, functions);
         }},
        {PAD_RIGHT, [&functions, &programState, &bufferState]() {
             return updateAbstractState(PAD_RIGHT, programState, bufferState, functions);
         }},
        {PAD_UP, [&functions, &programState, &bufferState]() {
             return updateAbstractState(PAD_UP, programState, bufferState, functions);
         }},
        {PAD_DOWN, [&functions, &programState, &bufferState]() {
             return updateAbstractState(PAD_DOWN, programState, bufferState, functions);
         }},
    };
    const auto INPUT_TO_LOGIC_AFTER = std::unordered_map<int, std::function<bool()>>{
        {L1, [&functions, &programState, resScalingX, resScalingY]() { auto [x, y] = BOARD_COORDINATES[programState.boardRow][programState.boardColumn];
            functions.moveMouse(x, y, resScalingX, resScalingY); return true; }},
        {R1, [&functions, &programState, resScalingX, resScalingY]() { auto [x, y] = BOARD_COORDINATES[programState.boardRow][programState.boardColumn];
            functions.moveMouse(x, y, resScalingX, resScalingY); return true; }},
        {X, [&functions, &programState, resScalingX, resScalingY]() { programState.mode = Mode::DRAW; auto [x, y] = DRAW_YES_NO[0][programState.drawColumn];
            functions.moveMouse(x, y, resScalingX, resScalingY); return true; }},
        {Y, [&functions, &programState, resScalingX, resScalingY]() { programState.mode = Mode::RESIGN; auto [x, y] = RESIGN_YES_NO[0][programState.resignColumn];
            functions.moveMouse(x, y, resScalingX, resScalingY); return true; }},
        {A, [&programState]() { if (programState.mode != Mode::BOARD) {programState.mode = Mode::BOARD; programState.drawColumn = 0; programState.resignColumn = 0;}
        return true; }},
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
                        functions.handleToMouseAbsoluteMove(input, PRESSED, resScalingX, resScalingY);
                    }
                    functions.handleToMouseAbsoluteMove(input, JUST_PRESSED, resScalingX, resScalingY);
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

} // namespace chess
