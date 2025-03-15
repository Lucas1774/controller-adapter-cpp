#include "joystick.h"

Joystick::Joystick(int xId, int yId, double deadZone, double sensitivity, double x, double y, bool isXActive, bool isYActive)
    : xId(xId), yId(yId), deadZone(deadZone), sensitivity(sensitivity), x(x), y(y), isXActive(isXActive), isYActive(isYActive) {}
