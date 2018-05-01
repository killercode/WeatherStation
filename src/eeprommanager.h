#include "Arduino.h"
#include <LinkedList.h>
#include "Setting.h"

#ifndef eeprommanager_h
#define eeprommanager_h

class EEPROMManager
{
    public:
        EEPROMManager();
        void readRecords();
        void writeRecords();
        void setSetting(String key, String value);
        void removeSetting(String key);
        String getSetting(String key);
        bool hasSetting(String key);
        LinkedList<Setting> settings = LinkedList<Setting>();

    private:
        String _recordRaw;
        String _raw;
        int _initialAddress;
        void parseRecords();
};

#endif