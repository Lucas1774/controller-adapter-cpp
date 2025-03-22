#include "funcs.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <windows.h>

namespace functions {
using enum Buttons;
using enum ButtonState;
using enum ButtonGroups;

void run(GameParams &gameParams) {
    try {
        while (true) {
            const auto loopStartTime = std::chrono::steady_clock::now();
            std::vector<SDL_Event> events;
            SDL_Event eventBuffer;
            while (SDL_PollEvent(&eventBuffer)) {
                events.push_back(eventBuffer);
            }
            if (!gameParams.running) {
                for (const auto &event : events) {
                    if (event.type == SDL_JOYBUTTONDOWN) {
                        const Buttons button = gameParams.buttonMapping.at(event.jbutton.button);
                        if (button == ACTIVATE) {
                            gameParams.running = true;
                            break;
                        }
                    }
                }
            } else {
                functions::state::updateNonAnalogState(gameParams.buttonState, events, gameParams.buttonMapping);
                if (gameParams.buttonState[ACTIVATE] == JUST_PRESSED) {
                    gameParams.running = false;
                    continue;
                }
                functions::state::updateJoystick(gameParams.buttonState, gameParams.joystick, gameParams.leftJoystick, LEFT_JS);
                functions::state::updateJoystick(gameParams.buttonState, gameParams.joystick, gameParams.rightJoystick, RIGHT_JS);
                if (gameParams.triggers != nullptr) {
                    functions::state::updateJoystick(gameParams.buttonState, gameParams.joystick, *gameParams.triggers, TRIGGERS);
                }
                functions::action::runMappings(gameParams.mappings, gameParams.resScalingX, gameParams.resScalingY, gameParams.turboInputs);
            }

            std::this_thread::sleep_for(std::chrono::microseconds(std::max(
                10000 - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - loopStartTime).count(), 0LL)));
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

namespace {

const std::unordered_map<int, int> BUTTON_ID_TO_PRESS_EVENT = {
    {SDL_BUTTON_LEFT, MOUSEEVENTF_LEFTDOWN},
    {SDL_BUTTON_RIGHT, MOUSEEVENTF_RIGHTDOWN},
    {SDL_BUTTON_MIDDLE, MOUSEEVENTF_MIDDLEDOWN}};
const std::unordered_map<int, int> BUTTON_ID_TO_RELEASE_EVENT = {
    {SDL_BUTTON_LEFT, MOUSEEVENTF_LEFTUP},
    {SDL_BUTTON_RIGHT, MOUSEEVENTF_RIGHTUP},
    {SDL_BUTTON_MIDDLE, MOUSEEVENTF_MIDDLEUP}};

void sendInput(const int key, const DWORD flags) {
    INPUT ip = {0};
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = static_cast<WORD>(MapVirtualKey(key, MAPVK_VK_TO_VSC));
    ip.ki.dwFlags = flags;
    SendInput(1, &ip, sizeof(INPUT));
}

bool actionCallbackBefore(const functions::Mappings &mappings, const Buttons &input) {
    const auto &map = mappings.input_to_conditioning_logic;
    if (const auto &action = map.find(input); action != map.end()) {
        return action->second();
    }
    return true;
}

void pressButton(const int button_to_press) {
    INPUT ip = {0};
    ip.type = INPUT_MOUSE;
    ip.mi.dwFlags = BUTTON_ID_TO_PRESS_EVENT.at(button_to_press);
    SendInput(1, &ip, sizeof(INPUT));
}

void releaseButton(const int button_to_release) {
    INPUT ip = {0};
    ip.type = INPUT_MOUSE;
    ip.mi.dwFlags = BUTTON_ID_TO_RELEASE_EVENT.at(button_to_release);
    SendInput(1, &ip, sizeof(INPUT));
}

void pressThenRelease(const int key_to_tap) {
    sendInput(key_to_tap, KEYEVENTF_SCANCODE);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sendInput(key_to_tap, KEYEVENTF_KEYUP);
}

void handleState(ButtonState &state, const bool is_pressed) {
    if (is_pressed) {
        if (state == RELEASED) {
            state = JUST_PRESSED;
        }
    } else {
        if (state == PRESSED) {
            state = JUST_RELEASED;
        }
    }
}

} // namespace

namespace action {

void runMappings(const Mappings &mappings, double resScalingX, double resScalingY, const std::unordered_set<Buttons> &turboInputs) {

    static const auto &runRawLogic = [&mappings, &turboInputs](const auto &mapping, const ButtonState eventType) {
        for (const auto &[input, logic] : mapping) {
            const ButtonState state = mappings.buttonState.at(input);
            if (eventType == JUST_PRESSED && turboInputs.find(input) != turboInputs.end() && state == PRESSED) {
                logic();
            }
            if (state == eventType) {
                logic();
            }
        }
    };

    static const auto &callHandler = [&mappings, &turboInputs](const auto &mapping, const auto &handler, const ButtonState eventType) {
        for (const auto &[input, _] : mapping) {
            if (eventType == JUST_PRESSED && turboInputs.find(input) != turboInputs.end()) {
                handler(mappings, input, PRESSED, -1);
            }
            handler(mappings, input, eventType, -1);
        }
    };

    static const auto &callHandlerWithResScaling = [&mappings, &turboInputs, resScalingX, resScalingY](const auto &mapping, const auto &handler, const ButtonState eventType) {
        for (const auto &[input, _] : mapping) {
            if (eventType == JUST_PRESSED && turboInputs.find(input) != turboInputs.end()) {
                handler(mappings, input, PRESSED, resScalingX, resScalingY);
            }
            handler(mappings, input, eventType, resScalingX, resScalingY);
        }
    };

    // logic before
    runRawLogic(mappings.input_to_logic_before, JUST_PRESSED);
    runRawLogic(mappings.release_to_logic_before, JUST_RELEASED);
    // standard
    callHandler(mappings.input_to_mouse_click, handleToClick, JUST_PRESSED);
    callHandler(mappings.release_to_mouse_click, handleToClick, JUST_RELEASED);
    callHandler(mappings.input_to_button_toggle, handleToButtonToggle, JUST_PRESSED);
    callHandler(mappings.release_to_button_toggle, handleToButtonToggle, JUST_RELEASED);
    callHandler(mappings.input_to_key_tap, handleToKeyTap, JUST_PRESSED);
    callHandler(mappings.release_to_key_tap, handleToKeyTap, JUST_RELEASED);
    // res scaled
    callHandlerWithResScaling(mappings.input_to_mouse_move, handleToMouseAbsoluteMove, JUST_PRESSED);
    callHandlerWithResScaling(mappings.release_to_mouse_move, handleToMouseAbsoluteMove, JUST_RELEASED);
    // event independent
    for (const auto &[input, _] : mappings.input_to_key_hold) {
        handleToKeyHold(mappings, input);
    }
    for (const auto &[buttonGroup, joystickSupplier] : mappings.joystick_to_mouse_relative) {
        handleJoystickToMouseRelative(mappings, buttonGroup, joystickSupplier(), resScalingX, resScalingY);
    }
    // logic after
    runRawLogic(mappings.input_to_logic_after, JUST_PRESSED);
    runRawLogic(mappings.release_to_logic_after, JUST_RELEASED);
}

void moveMouse(const int x, const int y, const double resScalingX, const double resScalingY) {
    SetCursorPos(static_cast<int>(x * resScalingX), static_cast<int>(y * resScalingY));
}

void moveMouseRelative(const int x, const int y, const double resScalingX, const double resScalingY) {
    POINT p;
    GetCursorPos(&p);
    SetCursorPos(static_cast<int>(p.x + x * resScalingX), static_cast<int>(p.y + y * resScalingY));
}

void click(const int button_to_click) {
    std::array<INPUT, 2> ip = {0};
    ip[0].type = INPUT_MOUSE;
    ip[0].mi.dwFlags = BUTTON_ID_TO_PRESS_EVENT.at(button_to_click);
    ip[1].type = INPUT_MOUSE;
    ip[1].mi.dwFlags = BUTTON_ID_TO_RELEASE_EVENT.at(button_to_click);
    SendInput(2, ip.data(), sizeof(INPUT));
}

void handleToMouseAbsoluteMove(const Mappings &mappings, const Buttons &input, const ButtonState eventType, const double resScalingX, const double resScalingY) {
    if (mappings.buttonState.at(input) == eventType && actionCallbackBefore(mappings, input)) {
        const auto [x, y] = PRESSED_STATES.contains(eventType) ? mappings.input_to_mouse_move.at(input)() : mappings.release_to_mouse_move.at(input)();
        moveMouse(x, y, resScalingX, resScalingY);
    }
}

void handleToClick(const Mappings &mappings, const Buttons &input, const ButtonState eventType, const int button) {
    if (mappings.buttonState.at(input) == eventType && actionCallbackBefore(mappings, input)) {
        int actualButton;
        if (button != -1) {
            actualButton = button;
        } else {
            actualButton = PRESSED_STATES.contains(eventType) ? mappings.input_to_mouse_click.at(input)() : mappings.release_to_mouse_click.at(input)();
        }
        click(actualButton);
    }
}

void handleToButtonToggle(const Mappings &mappings, const Buttons &input, const ButtonState eventType, const int button) {
    if (mappings.buttonState.at(input) == eventType && actionCallbackBefore(mappings, input)) {
        int actualButton;
        if (button != -1) {
            actualButton = button;
        } else {
            actualButton = PRESSED_STATES.contains(eventType) ? mappings.input_to_button_toggle.at(input)() : mappings.release_to_button_toggle.at(input)();
        }
        if (!(GetAsyncKeyState(actualButton) & 0x8000)) {
            pressButton(actualButton);
        } else {
            releaseButton(actualButton);
        }
    }
}

void handleToKeyTap(const Mappings &mappings, const Buttons &input, const ButtonState eventType, const int key) {
    if (mappings.buttonState.at(input) == eventType && actionCallbackBefore(mappings, input)) {
        int actualKey;
        if (key != -1) {
            actualKey = key;
        } else {
            actualKey = PRESSED_STATES.contains(eventType) ? mappings.input_to_key_tap.at(input)() : mappings.release_to_key_tap.at(input)();
        }
        std::jthread([actualKey] { pressThenRelease(actualKey); }).detach();
    }
}

void handleToKeyHold(const Mappings &mappings, const Buttons &input, const int key) {
    int eventFlag;
    switch (mappings.buttonState.at(input)) {
    case JUST_PRESSED:
        eventFlag = KEYEVENTF_SCANCODE;
        break;
    case JUST_RELEASED:
        eventFlag = KEYEVENTF_KEYUP;
        break;
    default:
        return;
    }
    if (actionCallbackBefore(mappings, input)) {
        const int actualKey = key != -1 ? key : mappings.input_to_key_hold.at(input)();
        sendInput(actualKey, eventFlag);
    }
}

void handleJoystickToMouseRelative(const Mappings &mappings, const ButtonGroups &input, const Joystick &joystick, const double resScalingX, const double resScalingY) {
    for (const auto &button : BUTTON_GROUP_TO_BUTTONS.at(input)) {
        if (PRESSED_STATES.contains(mappings.buttonState.at(button))) {
            if (actionCallbackBefore(mappings, button)) {
                functions::action::moveMouseRelative(
                    static_cast<int>(joystick.x * joystick.x * joystick.sensitivity * 100 * (joystick.x / std::abs(joystick.x))),
                    static_cast<int>(joystick.y * joystick.y * joystick.sensitivity * 100 * (joystick.y / std::abs(joystick.y))),
                    resScalingX, resScalingY);
            }
            return;
        }
    }
}

} // namespace action

namespace state {

void updateNonAnalogState(std::unordered_map<Buttons, ButtonState> &buttonState, const std::vector<SDL_Event> &events, const std::unordered_map<Uint8, Buttons> &buttonMapping) {
    for (auto &[_, state] : buttonState) {
        if (state == JUST_PRESSED) {
            state = PRESSED;
        } else if (state == JUST_RELEASED) {
            state = RELEASED;
        }
    }
    for (const auto &event : events) {
        switch (event.type) {
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP: {
            const Buttons button = buttonMapping.at(event.jbutton.button);
            handleState(buttonState.at(button), event.type == SDL_JOYBUTTONDOWN);
            break;
        }
        case SDL_JOYHATMOTION:
            handleState(buttonState.at(PAD_LEFT), event.jhat.value == SDL_HAT_LEFT);
            handleState(buttonState.at(PAD_RIGHT), event.jhat.value == SDL_HAT_RIGHT);
            handleState(buttonState.at(PAD_DOWN), event.jhat.value == SDL_HAT_DOWN);
            handleState(buttonState.at(PAD_UP), event.jhat.value == SDL_HAT_UP);
            break;
        default:
            break;
        }
    }
}

void updateJoystick(std::unordered_map<Buttons, ButtonState> &buttonState, SDL_Joystick *joystick, Joystick &meta, const ButtonGroups type) {
    switch (type) {
    case LEFT_JS:
    case RIGHT_JS: {
        const auto &buttons = BUTTON_GROUP_TO_BUTTONS.at(type);
        meta.x = SDL_JoystickGetAxis(joystick, meta.xId) / 32768.0f;
        meta.y = SDL_JoystickGetAxis(joystick, meta.yId) / 32768.0f;
        handleState(buttonState.at(buttons[0]), meta.x < -meta.deadZone);
        handleState(buttonState.at(buttons[1]), meta.x > meta.deadZone);
        handleState(buttonState.at(buttons[2]), meta.y < -meta.deadZone);
        handleState(buttonState.at(buttons[3]), meta.y > meta.deadZone);
        break;
    }
    case TRIGGERS:
        meta.x = (SDL_JoystickGetAxis(joystick, meta.xId) + 32768) / 65536.0f;
        meta.y = (SDL_JoystickGetAxis(joystick, meta.yId) + 32768) / 65536.0f;
        handleState(buttonState.at(L2), meta.x > meta.deadZone);
        handleState(buttonState.at(R2), meta.y > meta.deadZone);
        break;
    default:
        throw std::invalid_argument("Invalid joystick");
    }
}

} // namespace state

namespace abstractStateUtils {

bool isBufferFree(const std::unordered_map<Buttons, ButtonState> &buttonState, const int second_input_delay_mills,
                  const int subsequent_inputs_delay_millis, const Buttons &button, BufferState &bufferState) {
    const auto now = std::chrono::steady_clock::now();
    if (now - bufferState.lastExecuted <= std::chrono::milliseconds(subsequent_inputs_delay_millis)) {
        return false;
    } else if (!bufferState.isUnleashed && now - bufferState.lastPressed <= std::chrono::milliseconds(second_input_delay_mills)) {
        return false;
    } else {
        bufferState.lastExecuted = now;
        const ButtonState state = buttonState.at(button);
        bufferState.isUnleashed = (state == PRESSED);
        if (state == JUST_PRESSED) {
            bufferState.lastPressed = now;
        }
        return true;
    }
}

int generateAxisTargetWithBitMask(std::unordered_map<Buttons, ButtonState> &buttonState, const ButtonGroups eightAxis) {
    static constexpr int LEFT_MASK = 1;
    static constexpr int RIGHT_MASK = 2;
    static constexpr int UP_MASK = 4;
    static constexpr int DOWN_MASK = 8;

    static const std::unordered_map<int, int> DIRECTION_TO_MOVE_INDEX = {
        {LEFT_MASK | DOWN_MASK, 0},
        {LEFT_MASK, 1},
        {LEFT_MASK | UP_MASK, 2},
        {UP_MASK, 3},
        {RIGHT_MASK | DOWN_MASK, 6},
        {RIGHT_MASK, 5},
        {RIGHT_MASK | UP_MASK, 4},
        {DOWN_MASK, 7}};

    const auto &buttons = BUTTON_GROUP_TO_BUTTONS.at(eightAxis);
    const bool left = PRESSED_STATES.contains(buttonState.at(buttons[0]));
    const bool right = PRESSED_STATES.contains(buttonState.at(buttons[1]));
    const bool up = PRESSED_STATES.contains(buttonState.at(buttons[2]));
    const bool down = PRESSED_STATES.contains(buttonState.at(buttons[3]));
    return DIRECTION_TO_MOVE_INDEX.at((left ? LEFT_MASK : 0) | (right ? RIGHT_MASK : 0) | (up ? UP_MASK : 0) | (down ? DOWN_MASK : 0));
}

bool computeGridBasedTarget(const int rowCount, const int columnCount, int &rowIndex, int &columnIndex, const Buttons input) {
    switch (input) {
    case PAD_UP:
        if (1 == rowCount) {
            return false;
        }
        rowIndex = (rowIndex + rowCount - 1) % rowCount;
        return true;
    case PAD_DOWN:
        if (1 == rowCount) {
            return false;
        }
        rowIndex = (rowIndex + 1) % rowCount;
        return true;
    case PAD_LEFT:
        if (1 == columnCount) {
            return false;
        }
        columnIndex = (columnIndex + columnCount - 1) % columnCount;
        return true;
    case PAD_RIGHT:
        if (1 == columnCount) {
            return false;
        }
        columnIndex = (columnIndex + 1) % columnCount;
        return true;
    default:
        throw std::invalid_argument("Invalid button");
    }
}

} // namespace abstractStateUtils

} // namespace functions
