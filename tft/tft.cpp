#include "funcs.h"
#include "gameRegistry.h"

using enum Buttons;
using enum ButtonState;
using enum ButtonGroups;

enum class MouseMovementWithPadMode {
    BOARD,
    ITEMS,
    SHOP,
    CARDS,
    LOCK,
    FREE,
};

using enum MouseMovementWithPadMode;

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
    MouseMovementWithPadMode previousMode;
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
static constexpr std::array<std::array<Buttons, 3>, 3> LOCK_ADJACENCY_MATRIX = {
    {{NONE, PAD_DOWN, PAD_UP},   // lock
     {PAD_UP, NONE, PAD_DOWN},   // 1
     {PAD_DOWN, PAD_UP, NONE}}}; // 2

static bool updateAbstractState(const Buttons button, State &state, const std::unordered_map<Buttons, ButtonState> &buttonState, BufferState &bufferState) {
    if (!functions::abstractStateUtils::isBufferFree(buttonState, DEFAULT_SECOND_INPUT_DELAY_MILLIS, DEFAULT_SUBSEQUENT_INPUT_DELAY_MILLIS, button, bufferState)) {
        return false;
    }

    if (state.mode == FREE) {
        state.mode = BOARD;
        state.boardRow = 0;
        state.boardColumn = 0;
        return true;
    }

    if (state.mode == BOARD) {
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
        return true;
    }

    static const std::unordered_map<MouseMovementWithPadMode, std::function<bool()>> modeToFunction = {
        {ITEMS, [&state, &button]() {
             return functions::abstractStateUtils::computeGridBasedTarget(ITEM_COORDINATES.size(), ITEM_COORDINATES[0].size(), state.itemRow, state.itemColumn, button);
         }},
        {SHOP, [&state, &button]() {
             return functions::abstractStateUtils::computeGridBasedTarget(SHOP_COORDINATES.size(), SHOP_COORDINATES[0].size(), state.shopRow, state.shopColumn, button);
         }},
        {CARDS, [&state, &button]() {
             return functions::abstractStateUtils::computeGridBasedTarget(CARD_COORDINATES.size(), CARD_COORDINATES[0].size(), state.cardRow, state.cardColumn, button);
         }},
        {LOCK, [&state, &button]() {
             return functions::abstractStateUtils::computeAdjacencyMatrixBasedTarget(LOCK_ADJACENCY_MATRIX, state.lockIndex, button);
         }}};

    return modeToFunction.at(state.mode)();
}

static std::pair<int, int> getMouseTarget(const State &state) {

    static const std::unordered_map<MouseMovementWithPadMode, std::function<std::pair<int, int>()>> modeToStateDependingCoordinates = {
        {BOARD, [&state] { return BOARD_COORDINATES[state.boardRow][state.boardColumn]; }},
        {ITEMS, [&state] { return ITEM_COORDINATES[state.itemRow][state.itemColumn]; }},
        {SHOP, [&state] { return SHOP_COORDINATES[state.shopRow][state.shopColumn]; }},
        {CARDS, [&state] { return CARD_COORDINATES[state.cardRow][state.cardColumn]; }},
        {LOCK, [&state] { return LOCK_COORDINATES[state.lockIndex]; }}};

    return modeToStateDependingCoordinates.at(state.mode)();
}

namespace gameRegistry {

void runTft(const GameParams &params) {
    auto [buttonMapping, buttonState, running, resScalingX, resScalingY, joystick, leftJoystick, rightJoystick, triggers] = params;
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
        .mode = BOARD,
        .previousMode = BOARD};
    BufferState buffer_state = {
        .lastPressed = now,
        .lastExecuted = now,
        .isUnleashed = false};

    const auto TURBO_INPUTS = std::unordered_set<Buttons>{PAD_LEFT, PAD_RIGHT, PAD_UP, PAD_DOWN, LEFT_JS_LEFT, LEFT_JS_RIGHT, LEFT_JS_UP, LEFT_JS_DOWN};
    const auto INPUT_TO_KEY_TAP = std::unordered_map<Buttons, std::function<int()>>{
        {B, [] { return 'E'; }},
        {X, [] { return 'F'; }},
        {Y, [] { return 'D'; }},
        {R2, [] { return 'R'; }},
        {L2, [] { return 'Q'; }},
        {START, [] { return 'W'; }}};
    const auto JOYSTICK_TO_MOUSE_RELATIVE = std::unordered_map<ButtonGroups, std::function<Joystick &()>>{
        {RIGHT_JS, [&rightJoystick]() -> Joystick & { return rightJoystick; }}};
    const auto INPUT_TO_MOUSE_CLICK = std::unordered_map<Buttons, std::function<int()>>{
        {SELECT, [] { return SDL_BUTTON_RIGHT; }}};
    const auto INPUT_TO_BUTTON_TOGGLE = std::unordered_map<Buttons, std::function<int()>>{
        {A, []() { return SDL_BUTTON_LEFT; }}};
    const auto INPUT_TO_MOUSE_MOVE = std::unordered_map<Buttons, std::function<std::pair<int, int>()>>{
        {PAD_LEFT, [&state] { return getMouseTarget(state); }},
        {PAD_RIGHT, [&state] { return getMouseTarget(state); }},
        {PAD_UP, [&state] { return getMouseTarget(state); }},
        {PAD_DOWN, [&state] { return getMouseTarget(state); }},
        {R1, [&state] { return getMouseTarget(state); }},
        {L1, [&state] { return getMouseTarget(state); }},
        {R3, [&state] { return getMouseTarget(state); }},
        {L3, [&state] { return getMouseTarget(state); }}};
    const auto RELEASE_TO_MOUSE_MOVE = std::unordered_map<Buttons, std::function<std::pair<int, int>()>>{
        {R1, [&state] { return getMouseTarget(state); }},
        {L1, [&state] { return getMouseTarget(state); }}};
    const auto INPUT_TO_CONDITIONAL_LOGIC = std::unordered_map<Buttons, std::function<bool()>>{
        {A, [&state]() {if (state.mode == ITEMS) { return true; } functions::action::click(SDL_BUTTON_LEFT); return false; }},
        {PAD_LEFT, [&state, &buttonState, &buffer_state]() { return updateAbstractState(PAD_LEFT, state, buttonState, buffer_state); }},
        {PAD_RIGHT, [&state, &buttonState, &buffer_state]() { return updateAbstractState(PAD_RIGHT, state, buttonState, buffer_state); }},
        {PAD_UP, [&state, &buttonState, &buffer_state]() { return updateAbstractState(PAD_UP, state, buttonState, buffer_state); }},
        {PAD_DOWN, [&state, &buttonState, &buffer_state]() { return updateAbstractState(PAD_DOWN, state, buttonState, buffer_state); }}};
    const auto INPUT_TO_LOGIC_BEFORE = std::unordered_map<Buttons, std::function<void()>>{
        {L1, [&state]() { state.mode = ITEMS; }},
        {R1, [&state]() { state.mode = SHOP; state.shopColumn = 2; }},
        {R3, [&state]() {
             if (state.mode == CARDS) {
                 state.mode = BOARD;
                 state.previousMode = BOARD;
                 state.boardRow = 0;
                 state.boardColumn = 0;
             } else {
                 state.mode = CARDS;
                 state.previousMode = CARDS;
                 state.cardRow = 0;
                 state.cardColumn = 1;
             }
         }},
        {L3, [&state]() {
             if (state.mode == LOCK) {
                 state.mode = BOARD;
                 state.boardRow = 0;
                 state.boardColumn = 0;
             } else {
                 state.mode = LOCK;
                 state.lockIndex = 0;
             }
         }},
        {LEFT_JS_LEFT, [&buttonState, resScalingX, resScalingY] { auto [x, y] = MOVE_COORDINATES[functions::abstractStateUtils::generateAxisTargetWithBitMask(buttonState, LEFT_JS)];
            functions::action::moveMouse(x, y, resScalingX, resScalingY); }},
        {LEFT_JS_RIGHT, [&buttonState, resScalingX, resScalingY] { auto [x, y] = MOVE_COORDINATES[functions::abstractStateUtils::generateAxisTargetWithBitMask(buttonState, LEFT_JS)];
            functions::action::moveMouse(x, y, resScalingX, resScalingY); }},
        {LEFT_JS_UP, [&buttonState, resScalingX, resScalingY] { auto [x, y] = MOVE_COORDINATES[functions::abstractStateUtils::generateAxisTargetWithBitMask(buttonState, LEFT_JS)];
            functions::action::moveMouse(x, y, resScalingX, resScalingY); }},
        {LEFT_JS_DOWN, [&buttonState, resScalingX, resScalingY] { auto [x, y] = MOVE_COORDINATES[functions::abstractStateUtils::generateAxisTargetWithBitMask(buttonState, LEFT_JS)];
            functions::action::moveMouse(x, y, resScalingX, resScalingY); }}};
    const auto RELEASE_TO_LOGIC_BEFORE = std::unordered_map<Buttons, std::function<void()>>{
        {L1, [&state]() { state.mode = BOARD; state.previousMode = BOARD; }},
        {R1, [&state]() {
            state.mode = state.previousMode; if (state.mode == BOARD) {
                state.boardRow = 0;
                state.boardColumn = 0;
            } }}};
    const auto INPUT_TO_LOGIC_AFTER = std::unordered_map<Buttons, std::function<void()>>{
        {RIGHT_JS_LEFT, [&state]() { state.mode = FREE; }},
        {RIGHT_JS_RIGHT, [&state]() { state.mode = FREE; }},
        {RIGHT_JS_UP, [&state]() { state.mode = FREE; }},
        {RIGHT_JS_DOWN, [&state]() { state.mode = FREE; }},
        {LEFT_JS_LEFT, []() { functions::action::click(SDL_BUTTON_RIGHT); }},
        {LEFT_JS_RIGHT, []() { functions::action::click(SDL_BUTTON_RIGHT); }},
        {LEFT_JS_UP, []() { functions::action::click(SDL_BUTTON_RIGHT); }},
        {LEFT_JS_DOWN, []() { functions::action::click(SDL_BUTTON_RIGHT); }}};

    functions::Mappings mappings = {
        .buttonState = buttonState,
        .inputToMouseMove = INPUT_TO_MOUSE_MOVE,
        .releaseToMouseMove = RELEASE_TO_MOUSE_MOVE,
        .joystickToMouseRelative = JOYSTICK_TO_MOUSE_RELATIVE,
        .inputToMouseClick = INPUT_TO_MOUSE_CLICK,
        .inputToButtonToggle = INPUT_TO_BUTTON_TOGGLE,
        .inputToKeyTap = INPUT_TO_KEY_TAP,
        .inputToConditioningLogic = INPUT_TO_CONDITIONAL_LOGIC,
        .inputToLogicBefore = INPUT_TO_LOGIC_BEFORE,
        .releaseToLogicBefore = RELEASE_TO_LOGIC_BEFORE,
        .inputToLogicAfter = INPUT_TO_LOGIC_AFTER};

    functions::GameParams gameParams = {
        .buttonMapping = buttonMapping,
        .buttonState = buttonState,
        .joystick = joystick,
        .leftJoystick = leftJoystick,
        .rightJoystick = rightJoystick,
        .triggers = triggers,
        .mappings = mappings,
        .resScalingX = resScalingX,
        .resScalingY = resScalingY,
        .running = running,
        .turboInputs = TURBO_INPUTS};

    functions::run(gameParams);
}

} // namespace gameRegistry
