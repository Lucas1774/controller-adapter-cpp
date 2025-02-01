#include "joystick.h"

Joystick::Joystick(int xId, int yId, float deadZone, float sensitivity, float x, float y, bool isXActive, bool isYActive)
    : xId(xId), yId(yId), deadZone(deadZone), sensitivity(sensitivity), x(x), y(y), isXActive(isXActive), isYActive(isYActive) {}

Joystick readLeftJoystick(const Json::Value &config) {
    return Joystick(
        config["left_joystick_x_id"].asInt(),
        config["left_joystick_y_id"].asInt(),
        config["left_joystick_dead_zone"].asFloat(),
        config["left_joystick_sensitivity"].asFloat());
}

Joystick readRightJoystick(const Json::Value &config) {
    return Joystick(
        config["right_joystick_x_id"].asInt(),
        config["right_joystick_y_id"].asInt(),
        config["right_joystick_dead_zone"].asFloat(),
        config["right_joystick_sensitivity"].asFloat());
}

Joystick readTriggerJoystick(const Json::Value &config) {
    return Joystick(
        config["left_trigger_id"].asInt(),
        config["right_trigger_id"].asInt(),
        config["trigger_dead_zone"].asFloat(),
        config["trigger_sensitivity"].asFloat());
}

void initializeJoysticks(const Json::Value &config, Joystick *left, Joystick *right, Joystick *trigger) {
    if (left) {
        *left = readLeftJoystick(config);
    }
    if (right) {
        *right = readRightJoystick(config);
    }
    if (trigger) {
        *trigger = readTriggerJoystick(config);
    }
}