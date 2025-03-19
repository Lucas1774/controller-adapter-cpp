#include "funcs.h"
#include <stdexcept>
#include <thread>
#include <windows.h>

namespace functions {

namespace {

const std::unordered_map<int, DWORD> BUTTON_ID_TO_PRESS_EVENT = {
    {SDL_BUTTON_LEFT, MOUSEEVENTF_LEFTDOWN},
    {SDL_BUTTON_RIGHT, MOUSEEVENTF_RIGHTDOWN},
    {SDL_BUTTON_MIDDLE, MOUSEEVENTF_MIDDLEDOWN}};
const std::unordered_map<int, DWORD> BUTTON_ID_TO_RELEASE_EVENT = {
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

bool actionCallbackBefore(const functions::Mappings &mappings, const int &input) {
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

void handleState(int &state, const bool is_pressed) {
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

void runMappings(const Mappings &mappings, double resScalingX, double resScalingY, const std::unordered_set<int> &turboInputs) {

    static const auto &runRawLogic = [&mappings, &turboInputs](const auto &mapping, const int eventType) {
        for (const auto &[input, logic] : mapping) {
            const int state = mappings.buttonState.at(input);
            if (eventType == JUST_PRESSED && turboInputs.find(input) != turboInputs.end() && state == PRESSED) {
                logic();
            }
            if (state == eventType) {
                logic();
            }
        }
    };

    static const auto &callHandler = [&mappings, &turboInputs](const auto &mapping, const auto &handler, const int eventType) {
        for (const auto &[input, _] : mapping) {
            if (eventType == JUST_PRESSED && turboInputs.find(input) != turboInputs.end()) {
                handler(mappings, input, PRESSED, -1);
            }
            handler(mappings, input, eventType, -1);
        }
    };

    static const auto &callHandlerWithResScaling = [&mappings, &turboInputs](const auto &mapping, const auto &handler, const int eventType,
                                                                             const double resScalingX, const double resScalingY) {
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
    callHandlerWithResScaling(mappings.input_to_mouse_move, handleToMouseAbsoluteMove, JUST_PRESSED, resScalingX, resScalingY);
    callHandlerWithResScaling(mappings.release_to_mouse_move, handleToMouseAbsoluteMove, JUST_RELEASED, resScalingX, resScalingY);
    // event independent
    for (const auto &[input, _] : mappings.input_to_key_hold) {
        handleToKeyHold(mappings, input);
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

void handleToMouseAbsoluteMove(const Mappings &mappings, const int &input, const int eventType, const double resScalingX, const double resScalingY) {
    if (mappings.buttonState.at(input) == eventType) {
        if (actionCallbackBefore(mappings, input)) {
            const auto [x, y] = PRESSED_STATES.find(eventType) != PRESSED_STATES.end() ? mappings.input_to_mouse_move.at(input)() : mappings.release_to_mouse_move.at(input)();
            moveMouse(x, y, resScalingX, resScalingY);
        }
    }
}

void handleToClick(const Mappings &mappings, const int &input, const int eventType, const int button) {
    if (mappings.buttonState.at(input) == eventType) {
        if (actionCallbackBefore(mappings, input)) {
            int actualButton;
            if (button != -1) {
                actualButton = button;
            } else {
                actualButton = PRESSED_STATES.find(eventType) != PRESSED_STATES.end() ? mappings.input_to_mouse_click.at(input)() : mappings.release_to_mouse_click.at(input)();
            }
            click(actualButton);
        }
    }
}

void handleToButtonToggle(const Mappings &mappings, const int &input, const int eventType, const int button) {
    if (mappings.buttonState.at(input) == eventType) {
        if (actionCallbackBefore(mappings, input)) {
            int actualButton;
            if (button != -1) {
                actualButton = button;
            } else {
                actualButton = PRESSED_STATES.find(eventType) != PRESSED_STATES.end() ? mappings.input_to_button_toggle.at(input)() : mappings.release_to_button_toggle.at(input)();
            }
            if (!(GetAsyncKeyState(actualButton) & 0x8000)) {
                pressButton(actualButton);
            } else {
                releaseButton(actualButton);
            }
        }
    }
}

void handleToKeyTap(const Mappings &mappings, const int &input, const int eventType, const int key) {
    if (mappings.buttonState.at(input) == eventType) {
        if (actionCallbackBefore(mappings, input)) {
            int actualKey;
            if (key != -1) {
                actualKey = key;
            } else {
                actualKey = PRESSED_STATES.find(eventType) != PRESSED_STATES.end() ? mappings.input_to_key_tap.at(input)() : mappings.release_to_key_tap.at(input)();
            }
            std::thread([actualKey] { pressThenRelease(actualKey); }).detach();
        }
    }
}

void handleToKeyHold(const Mappings &mappings, const int &input, const int key) {
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

void listenToRunEvent(const std::vector<SDL_Event> &events,
                      const std::unordered_map<int, int> &buttonMapping,
                      bool &running) {
    for (const auto &event : events) {
        if (event.type == SDL_JOYBUTTONDOWN) {
            const int button = buttonMapping.at(event.jbutton.button);
            if (button == ACTIVATE) {
                running = true;
                return;
            }
        }
    }
}

void updateNonAnalogState(std::unordered_map<int, int> &buttonState, const std::vector<SDL_Event> &events, const std::unordered_map<int, int> &buttonMapping) {
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
            const int button = buttonMapping.at(event.jbutton.button);
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

void updateJoystickAsDigital(std::unordered_map<int, int> &buttonState, SDL_Joystick *joystick, Joystick &meta, const ButtonGroups type) {
    updateJoystickAsAnalog(joystick, meta, type);
    switch (type) {
    case LEFT_JS:
        handleState(buttonState.at(LEFT_JS_LEFT), meta.isXActive && meta.x < 0);
        handleState(buttonState.at(LEFT_JS_RIGHT), meta.isXActive && meta.x > 0);
        handleState(buttonState.at(LEFT_JS_UP), meta.isYActive && meta.y < 0);
        handleState(buttonState.at(LEFT_JS_DOWN), meta.isYActive && meta.y > 0);
        break;
    case RIGHT_JS:
        handleState(buttonState.at(RIGHT_JS_LEFT), meta.isXActive && meta.x < 0);
        handleState(buttonState.at(RIGHT_JS_RIGHT), meta.isXActive && meta.x > 0);
        handleState(buttonState.at(RIGHT_JS_UP), meta.isYActive && meta.y < 0);
        handleState(buttonState.at(RIGHT_JS_DOWN), meta.isYActive && meta.y > 0);
        break;
    case TRIGGERS:
        handleState(buttonState.at(L2), meta.isXActive);
        handleState(buttonState.at(R2), meta.isYActive);
        break;
    default:
        throw std::invalid_argument("Invalid joystick");
    }
}

void updateJoystickAsAnalog(SDL_Joystick *joystick, Joystick &meta, const ButtonGroups type) {
    switch (type) {
    case TRIGGERS:
        meta.x = (SDL_JoystickGetAxis(joystick, meta.xId) + 32768) / 65536.0f;
        meta.y = (SDL_JoystickGetAxis(joystick, meta.yId) + 32768) / 65536.0f;
        meta.isXActive = meta.x > meta.deadZone;
        meta.isYActive = meta.y > meta.deadZone;
        break;
    case LEFT_JS:
    case RIGHT_JS:
        meta.x = SDL_JoystickGetAxis(joystick, meta.xId) / 32768.0f;
        meta.y = SDL_JoystickGetAxis(joystick, meta.yId) / 32768.0f;
        meta.isXActive = std::abs(meta.x) > meta.deadZone;
        meta.isYActive = std::abs(meta.y) > meta.deadZone;
        break;
    default:
        throw std::invalid_argument("Invalid joystick");
    }
}

bool isBufferFree(const std::unordered_map<int, int> &buttonState, const int second_input_delay_mills,
                  const int subsequent_inputs_delay_millis, const int &button, BufferState &bufferState) {
    const auto now = std::chrono::steady_clock::now();
    if (now - bufferState.lastExecuted <= std::chrono::milliseconds(subsequent_inputs_delay_millis)) {
        return false;
    } else if (!bufferState.isUnleashed && now - bufferState.lastPressed <= std::chrono::milliseconds(second_input_delay_mills)) {
        return false;
    } else {
        bufferState.lastExecuted = now;
        const int state = buttonState.at(button);
        bufferState.isUnleashed = (state == PRESSED);
        if (state == JUST_PRESSED) {
            bufferState.lastPressed = now;
        }
        return true;
    }
}

int generateAxisTargetWithBitMask(std::unordered_map<int, int> &buttonState, const ButtonGroups eightAxis) {
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

    int bitmask, left, right, up, down;
    switch (eightAxis) {
    case LEFT_JS:
        left = PRESSED_STATES.find(buttonState.at(LEFT_JS_LEFT)) != PRESSED_STATES.end();
        right = PRESSED_STATES.find(buttonState.at(LEFT_JS_RIGHT)) != PRESSED_STATES.end();
        up = PRESSED_STATES.find(buttonState.at(LEFT_JS_UP)) != PRESSED_STATES.end();
        down = PRESSED_STATES.find(buttonState.at(LEFT_JS_DOWN)) != PRESSED_STATES.end();
        bitmask = (left * LEFT_MASK) | (right * RIGHT_MASK) | (up * UP_MASK) | (down * DOWN_MASK);
        return DIRECTION_TO_MOVE_INDEX.at(bitmask);
    case RIGHT_JS:
        left = PRESSED_STATES.find(buttonState.at(RIGHT_JS_LEFT)) != PRESSED_STATES.end();
        right = PRESSED_STATES.find(buttonState.at(RIGHT_JS_RIGHT)) != PRESSED_STATES.end();
        up = PRESSED_STATES.find(buttonState.at(RIGHT_JS_UP)) != PRESSED_STATES.end();
        down = PRESSED_STATES.find(buttonState.at(RIGHT_JS_DOWN)) != PRESSED_STATES.end();
        bitmask = (left * LEFT_MASK) | (right * RIGHT_MASK) | (up * UP_MASK) | (down * DOWN_MASK);
        return DIRECTION_TO_MOVE_INDEX.at(bitmask);
    case PAD:
        left = PRESSED_STATES.find(buttonState.at(PAD_LEFT)) != PRESSED_STATES.end();
        right = PRESSED_STATES.find(buttonState.at(PAD_RIGHT)) != PRESSED_STATES.end();
        up = PRESSED_STATES.find(buttonState.at(PAD_UP)) != PRESSED_STATES.end();
        down = PRESSED_STATES.find(buttonState.at(PAD_DOWN)) != PRESSED_STATES.end();
        bitmask = (left * LEFT_MASK) | (right * RIGHT_MASK) | (up * UP_MASK) | (down * DOWN_MASK);
        return DIRECTION_TO_MOVE_INDEX.at(bitmask);
    default:
        throw std::invalid_argument("Invalid button group");
    }
}

bool computeGridBasedTarget(const int rowCount, const int columnCount, int &rowIndex, int &columnIndex, const int button) {
    switch (button) {
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

} // namespace functions
