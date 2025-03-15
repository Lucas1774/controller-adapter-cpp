#ifndef JOYSTICK_H
#define JOYSTICK_H

struct Joystick {
    int xId;
    int yId;
    double deadZone;
    double sensitivity;
    double x;
    double y;
    bool isXActive;
    bool isYActive;

    Joystick(int xId = -1, int yId = -1, double deadZone = 0,
             double sensitivity = 0.0, double x = 0.0, double y = 0.0,
             bool isXActive = false, bool isYActive = false);
};

#endif // JOYSTICK_H
