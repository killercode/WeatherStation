#include "Arduino.h"
#include "Setting.h"

Setting::Setting()
{
}

Setting::Setting(String k, String v)
{
    key = k;
    value = v;
}

String Setting::getKey()
{
    return key;
}

void Setting::setKey(String k)
{
    key = k;
}

String Setting::getValue()
{
    return value;
}

void Setting::setValue(String v)
{
    value = v;
}
