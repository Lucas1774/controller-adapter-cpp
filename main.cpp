#define SDL_MAIN_HANDLED

#include "configParser.h"
#include "gameRegistry.h"
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
    using enum Buttons;
    using enum ButtonState;
    if (argc < 2) {
        std::cerr << "No game was specified" << std::endl;
        return 1;
    }
    std::string game = argv[1];

    Json::Value config;
    if (std::ifstream configFile("config.json", std::ifstream::binary); configFile.is_open()) {
        configFile >> config;
        configFile.close();
    } else {
        std::cerr << "Error opening config file." << std::endl;
        return 1;
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "SDL initialization error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
        std::cerr << "SDL display mode error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    const double resScalingX = dm.w / 1920.0;
    const double resScalingY = dm.h / 1080.0;
    SDL_Joystick *joystick = nullptr;
    bool hasAnalogTriggers = false;
    if (SDL_NumJoysticks() > 0) {
        joystick = SDL_JoystickOpen(0);
        SDL_PollEvent(nullptr);
        hasAnalogTriggers = SDL_JoystickGetAxis(joystick, config["left_trigger_id"].asInt()) != 0;
    } else {
        std::cerr << "No controller found" << std::endl;
        SDL_Quit();
        return 1;
    }
    std::unordered_map<Buttons, ButtonState> buttonState = {
        {A, RELEASED}, {B, RELEASED}, {X, RELEASED}, {Y, RELEASED}, {L1, RELEASED}, {R1, RELEASED}, {SELECT, RELEASED}, {START, RELEASED}, {L3, RELEASED}, {R3, RELEASED}, {XBOX, RELEASED}, {PAD_UP, RELEASED}, {PAD_LEFT, RELEASED}, {PAD_RIGHT, RELEASED}, {PAD_DOWN, RELEASED}, {L2, RELEASED}, {R2, RELEASED}, {ACTIVATE, RELEASED}, {LEFT_JS_LEFT, RELEASED}, {LEFT_JS_RIGHT, RELEASED}, {LEFT_JS_UP, RELEASED}, {LEFT_JS_DOWN, RELEASED}, {RIGHT_JS_LEFT, RELEASED}, {RIGHT_JS_RIGHT, RELEASED}, {RIGHT_JS_UP, RELEASED}, {RIGHT_JS_DOWN, RELEASED}};

    std::unordered_map<Uint8, Buttons> buttonMapping = configParser::readButtonMapping(config);
    bool running = configParser::readRunAutomatically(config);
    Joystick leftJoystick, rightJoystick, triggers;
    Joystick *triggersPtr = hasAnalogTriggers ? &triggers : nullptr;
    configParser::initializeJoysticks(config, leftJoystick, rightJoystick, triggersPtr);

    gameRegistry::GameParams params = {
        .buttonMapping = buttonMapping,
        .buttonState = buttonState,
        .running = running,
        .resScalingX = resScalingX,
        .resScalingY = resScalingY,
        .joystick = joystick,
        .leftJoystick = leftJoystick,
        .rightJoystick = rightJoystick,
        .triggers = triggersPtr};

    gameRegistry::runGame(game, params);

    if (joystick) {
        SDL_JoystickClose(joystick);
    }
    SDL_Quit();

    return 0;
}
