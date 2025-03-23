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

void run(const GameParams &gameParams) {
    auto [buttonMapping, buttonState, joystick, leftJoystick, rightJoystick, triggers, mappings, resScalingX, resScalingY, running, turboInputs] = gameParams;
    try {
        while (true) {
            const auto loopStartTime = std::chrono::steady_clock::now();
            std::vector<SDL_Event> events;
            SDL_Event eventBuffer;
            while (SDL_PollEvent(&eventBuffer)) {
                events.push_back(eventBuffer);
            }
            if (!running) {
                for (const auto &event : events) {
                    if (event.type == SDL_JOYBUTTONDOWN) {
                        const Buttons button = buttonMapping.at(event.jbutton.button);
                        if (button == ACTIVATE) {
                            running = true;
                            break;
                        }
                    }
                }
            } else {
                functions::state::updateNonAnalogState(buttonState, events, buttonMapping);
                if (buttonState[ACTIVATE] == JUST_PRESSED) {
                    running = false;
                    continue;
                }
                functions::state::updateJoystick(buttonState, joystick, leftJoystick, LEFT_JS);
                functions::state::updateJoystick(buttonState, joystick, rightJoystick, RIGHT_JS);
                if (triggers) {
                    functions::state::updateJoystick(buttonState, joystick, *triggers, TRIGGERS);
                }
                functions::action::runMappings(mappings, resScalingX, resScalingY, turboInputs);
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

void sendInput(const int key, const int flags) {
    INPUT ip = {0};
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = static_cast<WORD>(MapVirtualKey(key, MAPVK_VK_TO_VSC));
    ip.ki.dwFlags = flags;
    SendInput(1, &ip, sizeof(INPUT));
}

bool actionCallbackBefore(const std::unordered_map<Buttons, std::function<bool()>> &callbackMaps, const Buttons &input) {
    if (const auto &action = callbackMaps.find(input); action != callbackMaps.end()) {
        return action->second();
    }
    return true;
}

bool actionIsTurbo(const std::unordered_map<Buttons, ButtonState> &buttonState, const std::unordered_set<Buttons> &turboInputs, const Buttons &input) {
    return turboInputs.contains(input) && buttonState.at(input) == PRESSED;
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
    auto &callbacksMap = mappings.inputToConditioningLogic;
    auto &buttonState = mappings.buttonState;

    for (const auto &[input, logic] : mappings.inputToLogicBefore) {
        if (buttonState.at(input) == JUST_PRESSED || actionIsTurbo(buttonState, turboInputs, input)) {
            logic();
        }
    }
    for (const auto &[input, logic] : mappings.releaseToLogicBefore) {
        if (buttonState.at(input) == JUST_RELEASED) {
            logic();
        }
    }
    for (const auto &[input, coordinateSupplier] : mappings.inputToMouseMove) {
        if ((buttonState.at(input) == JUST_PRESSED || actionIsTurbo(buttonState, turboInputs, input)) && actionCallbackBefore(callbacksMap, input)) {
            auto [x, y] = coordinateSupplier();
            moveMouse(x, y, resScalingX, resScalingY);
        }
    }
    for (const auto &[input, coordinateSupplier] : mappings.releaseToMouseMove) {
        if (buttonState.at(input) == JUST_RELEASED && actionCallbackBefore(callbacksMap, input)) {
            auto [x, y] = coordinateSupplier();
            moveMouse(x, y, resScalingX, resScalingY);
        }
    }
    for (const auto &[input, buttonSupplier] : mappings.inputToMouseClick) {
        if ((buttonState.at(input) == JUST_PRESSED || actionIsTurbo(buttonState, turboInputs, input)) && actionCallbackBefore(callbacksMap, input)) {
            click(buttonSupplier());
        }
    }
    for (const auto &[input, buttonSupplier] : mappings.releaseToMouseClick) {
        if (buttonState.at(input) == JUST_RELEASED && actionCallbackBefore(callbacksMap, input)) {
            click(buttonSupplier());
        }
    }
    for (const auto &[input, buttonSupplier] : mappings.inputToButtonToggle) {
        if ((buttonState.at(input) == JUST_PRESSED || actionIsTurbo(buttonState, turboInputs, input)) && actionCallbackBefore(callbacksMap, input)) {
            int key = buttonSupplier();
            if (!(GetAsyncKeyState(key) & 0x8000)) {
                pressButton(key);
            } else {
                releaseButton(key);
            }
        }
    }
    for (const auto &[input, buttonSupplier] : mappings.releaseToButtonToggle) {
        if (buttonState.at(input) == JUST_RELEASED && actionCallbackBefore(callbacksMap, input)) {
            int key = buttonSupplier();
            if (!(GetAsyncKeyState(key) & 0x8000)) {
                pressButton(key);
            } else {
                releaseButton(key);
            }
        }
    }
    for (const auto &[input, keySupplier] : mappings.inputToKeyTap) {
        if ((buttonState.at(input) == JUST_PRESSED || actionIsTurbo(buttonState, turboInputs, input)) && actionCallbackBefore(callbacksMap, input)) {
            int key = keySupplier();
            std::jthread([key] { pressThenRelease(key); }).detach();
        }
    }
    for (const auto &[input, keySupplier] : mappings.releaseToKeyTap) {
        if (buttonState.at(input) == JUST_RELEASED && actionCallbackBefore(callbacksMap, input)) {
            int key = keySupplier();
            std::jthread([key] { pressThenRelease(key); }).detach();
        }
    }
    for (const auto &[input, keySupplier] : mappings.inputToKeyHold) {
        int eventFlag;
        switch (buttonState.at(input)) {
        case JUST_PRESSED:
            eventFlag = KEYEVENTF_SCANCODE;
            break;
        case JUST_RELEASED:
            eventFlag = KEYEVENTF_KEYUP;
            break;
        default:
            continue;
        }
        if (actionCallbackBefore(callbacksMap, input)) {
            sendInput(keySupplier(), eventFlag);
        }
    }
    for (const auto &[inputGroup, joystickSupplier] : mappings.joystickToMouseRelative) {
        for (const auto &input : BUTTON_GROUP_TO_BUTTONS.at(inputGroup)) {
            if (PRESSED_STATES.contains(buttonState.at(input))) {
                if (actionCallbackBefore(callbacksMap, input)) {
                    const auto &joystick = joystickSupplier();
                    functions::action::moveMouseRelative(
                        static_cast<int>(joystick.x * joystick.x * joystick.sensitivity * 100 * (joystick.x / std::abs(joystick.x))),
                        static_cast<int>(joystick.y * joystick.y * joystick.sensitivity * 100 * (joystick.y / std::abs(joystick.y))),
                        resScalingX, resScalingY);
                }
                break;
            }
        }
    }
    for (const auto &[input, logic] : mappings.inputToLogicAfter) {
        if (buttonState.at(input) == JUST_PRESSED || actionIsTurbo(buttonState, turboInputs, input)) {
            logic();
        }
    }
    for (const auto &[input, logic] : mappings.releaseToLogicAfter) {
        if (buttonState.at(input) == JUST_RELEASED) {
            logic();
        }
    }
}

void moveMouse(const int x, const int y, const double resScalingX, const double resScalingY) {
    SetCursorPos(static_cast<int>(x * resScalingX), static_cast<int>(y * resScalingY));
}

void moveMouseRelative(const int x, const int y, const double resScalingX, const double resScalingY) {
    POINT p;
    GetCursorPos(&p);
    SetCursorPos(static_cast<int>(p.x + x * resScalingX), static_cast<int>(p.y + y * resScalingY));
}

void click(const int button) {
    std::array<INPUT, 2> ip = {0};
    ip[0].type = INPUT_MOUSE;
    ip[0].mi.dwFlags = BUTTON_ID_TO_PRESS_EVENT.at(button);
    ip[1].type = INPUT_MOUSE;
    ip[1].mi.dwFlags = BUTTON_ID_TO_RELEASE_EVENT.at(button);
    SendInput(2, ip.data(), sizeof(INPUT));
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

bool isBufferFree(const std::unordered_map<Buttons, ButtonState> &buttonState, const int secondInputDelayMillis,
                  const int subsequentInputDelayMillis, const Buttons &button, BufferState &bufferState) {
    const auto now = std::chrono::steady_clock::now();
    if (now - bufferState.lastExecuted <= std::chrono::milliseconds(subsequentInputDelayMillis)) {
        return false;
    } else if (!bufferState.isUnleashed && now - bufferState.lastPressed <= std::chrono::milliseconds(secondInputDelayMillis)) {
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
