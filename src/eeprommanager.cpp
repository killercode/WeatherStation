#include "Arduino.h"
#include <EEPROM.h>

#include "eeprommanager.h"

EEPROMManager::EEPROMManager()
{
    _initialAddress = 0;
}

bool EEPROMManager::hasSetting(String keyName)
{
    for(int i = 0; i < settings.size(); i++)
    {
        if (settings.get(i).getKey().equals(keyName))
        return true;
    }
    return false;
}

void EEPROMManager::readRecords()
{
    EEPROM.begin(1024);
    char arrayToStore[500];
    EEPROM.get(_initialAddress, arrayToStore);
   
    _raw = arrayToStore;
    parseRecords();
    EEPROM.end();
}

void EEPROMManager::writeRecords()
{
    EEPROM.begin(1024);
    
    _raw = "{";
    for(int i = 0; i < settings.size(); i++)
    {
        Setting s = settings.get(i);
        _raw = _raw + s.getKey();
        _raw = _raw + ':';
        _raw = _raw + s.getValue();
        _raw = _raw + ';';
    }
    _raw = _raw + '}';
    
    char arrayToStore[500];
    _raw.toCharArray(arrayToStore, _raw.length()+1);
    EEPROM.put(_initialAddress, arrayToStore);
    EEPROM.end();
}

void EEPROMManager::setSetting(String key, String value)
{
    bool bFound = false;
    for( int i = 0; i < settings.size(); i++)
    {
        if(settings.get(i).getKey().equals( key ))
        {
            Setting s = settings.get(i);
            s.setValue(value);
            settings.set(i,s);
            bFound = true;
            break;
        }
    }
    if (!bFound)
    {
        Setting s = Setting();
        s.setKey(key);
        s.setValue(value);
        settings.add(s);
    }
}

String EEPROMManager::getSetting(String key)
{
    for( int i = 0; i < settings.size(); i++)
    {
        if(settings.get(i).getKey().equals( key ))
        {
            return settings.get(i).getValue();
        }
    }
    return String();
}

void EEPROMManager::removeSetting(String key)
{
    for( int i = 0; i < settings.size(); i++ )
    {
        if(settings.get(i).getKey().equals( key ))
        {
            settings.remove(i);
        }
    }
}

void EEPROMManager::parseRecords()
{
    bool fStart = false;
    bool fIsKey = false;
    bool fIsValue = false;
    String key = String();
    String value = String();
    Setting s = Setting();

    for(int i = 0; i < 500; i++)
    {
        if (!fStart)
        {
            if(_raw[i] == '{')
            {
                fStart = true;
                fIsKey = true;
            }
        }
        else
        {
            if (_raw[i] == ':')
            {
                fIsKey = false;
                fIsValue = true;
                s.setKey(key);
                key = "";
                continue;
            }
            else if (_raw[i] == ';')
            {
                fIsKey = true;
                fIsValue = false;
                s.setValue(value);
                value = "";
                settings.add(s);
                s=Setting();
                continue;
            }
            else if (_raw[i] == '}')
            {
                fStart = false;
                break;
            }
            else
            {
                if (fIsKey)
                {
                    key = key + _raw[i];
                }
                else if(fIsValue)
                {
                    value = value + _raw[i];
                }
            }
        }
    }
}