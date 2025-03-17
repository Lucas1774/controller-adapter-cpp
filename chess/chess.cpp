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

static bool updateAbstractState(const int button, State &state, const std::unordered_map<int, int> &buttonState, BufferState &bufferState) {
    if (!functions::isBufferFree(buttonState, DEFAULT_SECOND_INPUT_DELAY_MILLIS, DEFAULT_SUBSEQUENT_INPUT_DELAY_MILLIS, button, bufferState)) {
        return false;
    }

    static const std::map<Mode, std::function<bool()>> modeToFunction = {
        {Mode::BOARD, [&state, &button]() {
             return functions::computeGridBasedTarget(BOARD_COORDINATES.size(), BOARD_COORDINATES[0].size(), state.boardRow, state.boardColumn, button);
         }},
        {Mode::RESIGN, [&state, &button]() {
             return functions::computeGridBasedTarget(RESIGN_YES_NO.size(), RESIGN_YES_NO[0].size(), state.resignRow, state.resignColumn, button);
         }},
        {Mode::DRAW, [&state, &button]() {
             return functions::computeGridBasedTarget(DRAW_YES_NO.size(), DRAW_YES_NO[0].size(), state.drawRow, state.drawColumn, button);
         }}};

    return modeToFunction.at(state.mode)();
}

static std::pair<int, int> getMouseTarget(const State &state) {

    static const std::unordered_map<Mode, std::function<std::pair<int, int>()>> modeToStateDependingCoordinates = {
        {Mode::BOARD, [&state] { return BOARD_COORDINATES[state.boardRow][state.boardColumn]; }},
        {Mode::RESIGN, [&state] { return RESIGN_YES_NO[state.resignRow][state.resignColumn]; }},
        {Mode::DRAW, [&state] { return DRAW_YES_NO[state.drawRow][state.drawColumn]; }}};

    return modeToStateDependingCoordinates.at(state.mode)();
}

void run(std::unordered_map<int, int> &buttonState,
         const Json::Value &config,
         const int screenWidth,
         const int screenHeight) {
    std::unordered_map<int, int> buttonMapping = configParser::readButtonMapping(config);
    bool running = configParser::readRunAutomatically(config);

    const double resScalingX = screenWidth / 1920.0;
    const double resScalingY = screenHeight / 1080.0;
    const auto now = std::chrono::steady_clock::now();

    State state = {
        .boardRow = 0,
        .boardColumn = 0,
        .resignRow = 0,
        .resignColumn = 0,
        .drawRow = 0,
        .drawColumn = 0,
        .mode = Mode::BOARD};
    BufferState bufferState = {
        .lastPressed = now,
        .lastExecuted = now,
        .isUnleashed = false};

    const auto TURBO_INPUTS = std::unordered_set<int>{PAD_LEFT, PAD_RIGHT, PAD_UP, PAD_DOWN};
    const auto INPUT_TO_MOUSE_MOVE = std::unordered_map<int, std::function<std::pair<int, int>()>>{
        {PAD_LEFT, [&state]() { return getMouseTarget(state); }},
        {PAD_RIGHT, [&state]() { return getMouseTarget(state); }},
        {PAD_UP, [&state]() { return getMouseTarget(state); }},
        {PAD_DOWN, [&state]() { return getMouseTarget(state); }}};
    const auto INPUT_TO_MOUSE_CLICK = std::unordered_map<int, std::function<int()>>{
        {B, []() { return SDL_BUTTON_RIGHT; }},
        {R1, []() { return SDL_BUTTON_LEFT; }},
        {L1, []() { return SDL_BUTTON_LEFT; }},
        {Y, []() { return SDL_BUTTON_LEFT; }},
        {X, []() { return SDL_BUTTON_LEFT; }}};
    const auto INPUT_TO_BUTTON_TOGGLE = std::unordered_map<int, std::function<int()>>{{A, []() { return SDL_BUTTON_LEFT; }}};
    const auto RELEASE_TO_BUTTON_TOGGLE = std::unordered_map<int, std::function<int()>>{{A, []() { return SDL_BUTTON_LEFT; }}};
    const auto INPUT_TO_LOGIC_BEFORE = std::unordered_map<int, std::function<bool()>>{
        {L1, [resScalingX, resScalingY]() { functions::moveMouse(REMATCH.first,REMATCH.second, resScalingX, resScalingY); return true; }},
        {R1, [resScalingX, resScalingY]() { functions::moveMouse(PLAY_AGAIN.first,PLAY_AGAIN.second, resScalingX, resScalingY); return true; }},
        {X, [resScalingX, resScalingY]() { functions::moveMouse(DRAW.first,DRAW.second, resScalingX, resScalingY); return true; }},
        {Y, [resScalingX, resScalingY]() { functions::moveMouse(RESIGN.first,RESIGN.second, resScalingX, resScalingY); return true; }},
        {PAD_LEFT, [&state, &buttonState, &bufferState]() { return updateAbstractState(PAD_LEFT, state, buttonState, bufferState); }},
        {PAD_RIGHT, [&state, &buttonState, &bufferState]() { return updateAbstractState(PAD_RIGHT, state, buttonState, bufferState); }},
        {PAD_UP, [&state, &buttonState, &bufferState]() { return updateAbstractState(PAD_UP, state, buttonState, bufferState); }},
        {PAD_DOWN, [&state, &buttonState, &bufferState]() { return updateAbstractState(PAD_DOWN, state, buttonState, bufferState); }}};
    const auto INPUT_TO_LOGIC_AFTER = std::unordered_map<int, std::function<void()>>{
        {L1, [&state, resScalingX, resScalingY]() { const auto [x, y] = BOARD_COORDINATES[state.boardRow][state.boardColumn];
            functions::moveMouse(x, y, resScalingX, resScalingY); }},
        {R1, [&state, resScalingX, resScalingY]() { const auto [x, y] = BOARD_COORDINATES[state.boardRow][state.boardColumn];
            functions::moveMouse(x, y, resScalingX, resScalingY); }},
        {X, [&state, resScalingX, resScalingY]() { state.mode = Mode::DRAW; const auto [x, y] = DRAW_YES_NO[0][state.drawColumn];
            functions::moveMouse(x, y, resScalingX, resScalingY); }},
        {Y, [&state, resScalingX, resScalingY]() { state.mode = Mode::RESIGN; const auto [x, y] = RESIGN_YES_NO[0][state.resignColumn];
            functions::moveMouse(x, y, resScalingX, resScalingY); }},
        {A, [&state]() { if (state.mode != Mode::BOARD) {state.mode = Mode::BOARD; state.drawColumn = 0; state.resignColumn = 0;}
        return true; }}};

    functions::Mappings mappings = {
        .buttonState = buttonState,
        .input_to_mouse_move = INPUT_TO_MOUSE_MOVE,
        .input_to_mouse_click = INPUT_TO_MOUSE_CLICK,
        .input_to_button_toggle = INPUT_TO_BUTTON_TOGGLE,
        .release_to_button_toggle = RELEASE_TO_BUTTON_TOGGLE,
        .input_to_logic_before = INPUT_TO_LOGIC_BEFORE,
        .input_to_logic_after = INPUT_TO_LOGIC_AFTER};

    try {
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
                for (const auto &[input, _] : INPUT_TO_BUTTON_TOGGLE) {
                    functions::handleToButtonToggle(mappings, input, JUST_PRESSED);
                }
                for (const auto &[input, _] : RELEASE_TO_BUTTON_TOGGLE) {
                    functions::handleToButtonToggle(mappings, input, JUST_RELEASED);
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(std::max(
                10000 - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - loopStartTime).count(), 0LL)));
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

} // namespace chess
