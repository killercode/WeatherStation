#include "Arduino.h"
#ifndef SRC_READING_H_
#define SRC_READING_H_

class Reading
{
 public:
        Reading();
        float GetTemperature();
        void SetTemperature(float value);
        float GetHumidity();
        void SetHumidity(float value0);
        float GetPressure();
        void SetPressure(float value1);
};
#endif  // SRC_READING_H_
