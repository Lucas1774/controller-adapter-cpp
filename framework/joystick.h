#ifndef JOYSTICK_H
#define JOYSTICK_H

struct Joystick {
    int xId;
    int yId;
    double deadZone;
    double sensitivity;
    double x;
    double y;

    explicit Joystick(const int xId = -1, const int yId = -1, const double deadZone = 0.0, const double sensitivity = 0.0, const double x = 0.0, const double y = 0.0)
        : xId(xId), yId(yId), deadZone(deadZone), sensitivity(sensitivity), x(x), y(y) {}
};

#endif // JOYSTICK_H
