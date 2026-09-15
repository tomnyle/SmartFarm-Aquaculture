#include "calibration.h"

float Calibration::linearMap(float input, float in_min, float in_max, float out_min, float out_max) {
    if (in_max == in_min) {
        return out_min;
    }
    return ((input - in_min) * (out_max - out_min) / (in_max - in_min)) + out_min;
}
