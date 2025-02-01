#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <json/json.h>

struct Joystick {
    int xId;
    int yId;
    float deadZone;
    float sensitivity;
    float x;
    float y;
    bool isXActive;
    bool isYActive;

    Joystick(int xId = -1, int yId = -1, float deadZone = 0, float sensitivity = 0.0, float x = 0.0, float y = 0.0, bool isXActive = false, bool isYActive = false);
};

void initializeJoysticks(const Json::Value &config, Joystick *left = nullptr, Joystick *right = nullptr, Joystick *trigger = nullptr);

#endif // JOYSTICK_H
