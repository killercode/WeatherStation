#include "Arduino.h"
#include "reading.h"

float temperature = 0;
float humidity = 0;
float pressure = 0;

Reading::Reading()
{
}

float Reading::GetTemperature()
{
    return temperature;
}

void Reading::SetTemperature(float value)
{
    temperature = value;
}

float Reading::GetHumidity()
{
    return humidity;
}

void Reading::SetHumidity(float value)
{
    humidity = value;
}

float Reading::GetPressure()
{
    return pressure;
}

void Reading::SetPressure(float value)
{
    pressure = value;
}
