#include "Arduino.h"
#include "reading.h"

float temperature = 0;
float humidity = 0;

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