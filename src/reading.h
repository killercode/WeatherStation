#include "Arduino.h"

#ifndef reading_h
#define reading_h

class Reading
{
    public:
        Reading();
        float GetTemperature();
        void SetTemperature(float value);
        float GetHumidity();
        void SetHumidity(float value0);
};
#endif