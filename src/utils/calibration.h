#ifndef SMARTFARM_CALIBRATION_H
#define SMARTFARM_CALIBRATION_H

class Calibration {
public:
    static float linearMap(float input, float in_min, float in_max, float out_min, float out_max);
};

#endif
