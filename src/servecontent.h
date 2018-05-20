#include "Arduino.h"
#include <ESP8266WebServer.h>
#include "eeprommanager.h"
#include "reading.h"

#ifndef SRC_SERVECONTENT_H_
#define SRC_SERVECONTENT_H_

class ServeContent
{
 public:
        ServeContent();
        ServeContent(ESP8266WebServer* pServer, EEPROMManager* eeprommanager, Reading* reading);
        void mainPage();
        void resetPassword();
        void wifiSetting();
        void loginPage();
        void handleNotFound();
        void getReadings();
        void noInternet();
        void restart();
 private:
        bool is_authenticated();
        bool handleFileRead(String path);
        String getContentType(String filename);
};
#endif  // SRC_SERVECONTENT_H_
