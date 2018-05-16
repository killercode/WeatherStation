#include "Arduino.h"
#include <ESP8266WebServer.h>
#include "eeprommanager.h"
#include "reading.h"

#ifndef servecontent_h
#define servecontent_h

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
    private:
        bool is_authenticated();
        bool handleFileRead(String path);
        String getContentType(String filename); 
        
};

#endif