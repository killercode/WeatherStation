#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "eeprommanager.h"
#include "servecontent.h"

ESP8266WebServer server(80);
EEPROMManager eepromsettings = EEPROMManager();
ServeContent serveContent;

void setup()
{
  //Workflow:
  //3-username and login will be admin/admin
  //4-open configuration page
  //5-after setting up it should reboot
  Serial.begin(115200);
  Serial.println("Starting...");
  eepromsettings.readRecords(); 
  bool bWifiFailed = false;
  Serial.println("Trying to fetch settings from EEPROM");
  if (eepromsettings.hasSetting("SSID") && eepromsettings.hasSetting("WIFIPASS"))
  {
    //Start WIFI
    WiFi.begin(eepromsettings.getSetting("SSID").c_str(), eepromsettings.getSetting("WIFIPASS").c_str());
    int failCounter = 0;
    while ((WiFi.status() != WL_CONNECTED) && ( failCounter < 30 ))
    {
      delay(500);
      Serial.print(".");
      failCounter++;
    }

    if (failCounter == 30)
    {
      Serial.println("Failed to connect to WIFI, will create access point!");
      bWifiFailed = true;
    }
    else
    {
      Serial.println("");
    }
  }
  else if ( !(eepromsettings.hasSetting("SSID") && eepromsettings.hasSetting("WIFIPASS") ) || (bWifiFailed) )
  {
    // TODO: FIX IP ADDRESS SETTINGS!!!!
    //Start Access Point
    IPAddress    apIP(192, 168, 1, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));   // subnet FF FF FF 00
    bool result = WiFi.softAP("MafraLabWS", "123456789");
    if (result)
    {
      Serial.println("Access Point Started");
    }
    else
    {
      Serial.println("Failed to start Access Point");
    }
  }

  serveContent=ServeContent(&server, &eepromsettings);
  Serial.println(WiFi.localIP());
  server.on("/", std::bind(&ServeContent::mainPage, serveContent));
  server.on("/login", std::bind(&ServeContent::loginPage, serveContent));
  server.on("/resetpassword", std::bind(&ServeContent::resetPassword, serveContent));
  server.on("/wifisettings", std::bind(&ServeContent::wifiSetting, serveContent));
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works without need of authentification");
  });

  server.onNotFound(std::bind(&ServeContent::handleNotFound, serveContent));
  //here the list of headers to be recorded
  const char *headerkeys[] = {"User-Agent", "Cookie"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  //ask server to track these headers
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  server.handleClient();
}
