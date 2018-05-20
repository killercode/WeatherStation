#include "Arduino.h"
#include <LinkedList.h>
#include "Setting.h"

#ifndef SRC_EEPROMMANAGER_H_
#define SRC_EEPROMMANAGER_H_

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

#endif  // SRC_EEPROMMANAGER_H_
