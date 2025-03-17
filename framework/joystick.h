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

    Joystick(const int xId = -1, const int yId = -1, const double deadZone = 0.0, const double sensitivity = 0.0,
             const double x = 0.0, const double y = 0.0, const bool isXActive = false, const bool isYActive = false)
        : xId(xId), yId(yId), deadZone(deadZone), sensitivity(sensitivity), x(x), y(y), isXActive(isXActive), isYActive(isYActive) {}
};

#endif // JOYSTICK_H
