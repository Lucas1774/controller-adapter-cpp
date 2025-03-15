#include "configParser.h"

namespace {

Joystick readLeftJoystick(const Json::Value &config) {
    return Joystick(
        config["left_joystick_x_id"].asInt(),
        config["left_joystick_y_id"].asInt(),
        config["left_joystick_dead_zone"].asDouble(),
        config["left_joystick_sensitivity"].asDouble());
}

Joystick readRightJoystick(const Json::Value &config) {
    return Joystick(
        config["right_joystick_x_id"].asInt(),
        config["right_joystick_y_id"].asInt(),
        config["right_joystick_dead_zone"].asDouble(),
        config["right_joystick_sensitivity"].asDouble());
}

Joystick readTriggerJoystick(const Json::Value &config) {
    return Joystick(
        config["left_trigger_id"].asInt(),
        config["right_trigger_id"].asInt(),
        config["trigger_dead_zone"].asDouble(),
        config["trigger_sensitivity"].asDouble());
}

} // anonymous namespace

namespace configParser {

std::unordered_map<int, int> readButtonMapping(const Json::Value &config) {
    std::unordered_map<int, int> buttonMapping;
    for (const std::string &configKey : config["button_mapping"].getMemberNames()) {
        int key = config["button_mapping"][configKey].asInt() - 1;
        buttonMapping[key] = BUTTON_NAME_TO_BUTTON_ID.at(configKey);
    }
    return buttonMapping;
}

void initializeJoysticks(const Json::Value &config, Joystick *left,
                         Joystick *right, Joystick *trigger) {
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

bool readRunAutomatically(const Json::Value &config) {
    return config["run_automatically"].asBool();
}

} // namespace configParser
