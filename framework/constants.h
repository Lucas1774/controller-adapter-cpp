#pragma once

#include <array>
#include <chrono>
#include <string>
#include <unordered_map>
#include <unordered_set>

static constexpr int DEFAULT_SECOND_INPUT_DELAY_MILLIS = 200;
static constexpr int DEFAULT_SUBSEQUENT_INPUT_DELAY_MILLIS = 50;
static constexpr int MILLIS_PER_FRAME = 16;
static constexpr int CENTER_X = 960;
static constexpr int CENTER_Y = 540;
static constexpr auto CENTER = std::make_pair(CENTER_X, CENTER_Y);

enum class Buttons {
    A,
    B,
    X,
    Y,
    L1,
    R1,
    SELECT,
    START,
    L3,
    R3,
    XBOX,
    PAD_UP,
    PAD_LEFT,
    PAD_RIGHT,
    PAD_DOWN,
    L2,
    R2,
    ACTIVATE,
    LEFT_JS_LEFT,
    LEFT_JS_RIGHT,
    LEFT_JS_UP,
    LEFT_JS_DOWN,
    RIGHT_JS_LEFT,
    RIGHT_JS_RIGHT,
    RIGHT_JS_UP,
    RIGHT_JS_DOWN,
    NONE
};

using enum Buttons;

enum class ButtonGroups { LEFT_JS,
                          RIGHT_JS,
                          PAD,
                          TRIGGERS };

using enum ButtonGroups;

const std::unordered_map<std::string, Buttons> BUTTON_NAME_TO_BUTTON_ID = {
    {"A", A},
    {"B", B},
    {"X", X},
    {"Y", Y},
    {"L1", L1},
    {"R1", R1},
    {"SELECT", SELECT},
    {"START", START},
    {"L3", L3},
    {"R3", R3},
    {"XBOX", XBOX},
    {"UP", PAD_UP},
    {"LEFT", PAD_LEFT},
    {"RIGHT", PAD_RIGHT},
    {"DOWN", PAD_DOWN},
    {"L2", L2},
    {"R2", R2},
    {"ACTIVATE", ACTIVATE},
    {"LEFT_JS_LEFT", LEFT_JS_LEFT},
    {"LEFT_JS_RIGHT", LEFT_JS_RIGHT},
    {"LEFT_JS_UP", LEFT_JS_UP},
    {"LEFT_JS_DOWN", LEFT_JS_DOWN},
    {"RIGHT_JS_LEFT", RIGHT_JS_LEFT},
    {"RIGHT_JS_RIGHT", RIGHT_JS_RIGHT},
    {"RIGHT_JS_UP", RIGHT_JS_UP},
    {"RIGHT_JS_DOWN", RIGHT_JS_DOWN}};

const std::unordered_map<ButtonGroups, std::array<Buttons, 4>> BUTTON_GROUP_TO_BUTTONS = {
    {LEFT_JS, std::array<Buttons, 4>{{LEFT_JS_LEFT, LEFT_JS_RIGHT, LEFT_JS_UP, LEFT_JS_DOWN}}},
    {RIGHT_JS, std::array<Buttons, 4>{{RIGHT_JS_LEFT, RIGHT_JS_RIGHT, RIGHT_JS_UP, RIGHT_JS_DOWN}}},
    {PAD, std::array<Buttons, 4>{{PAD_LEFT, PAD_RIGHT, PAD_UP, PAD_DOWN}}}};

enum class ButtonState {
    PRESSED,
    JUST_PRESSED,
    JUST_RELEASED,
    RELEASED,
};

using enum ButtonState;

struct BufferState {
    std::chrono::steady_clock::time_point lastPressed;
    std::chrono::steady_clock::time_point lastExecuted;
    bool isUnleashed;
};

const std::unordered_set<ButtonState> PRESSED_STATES = {PRESSED, JUST_PRESSED};
const std::unordered_set<ButtonState> RELEASED_STATES = {RELEASED, JUST_RELEASED};