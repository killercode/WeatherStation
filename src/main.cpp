#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ESP8266mDNS.h>
#include "eeprommanager.h"
#include "servecontent.h"
#include "pingClass.h"
#include "DHT.h"
#include <Ticker.h>
#include "reading.h"

Reading readSensors = Reading();
DHT dht;
ESP8266WebServer server(80);
EEPROMManager eepromsettings = EEPROMManager();
ServeContent serveContent;
PingClass Ping = PingClass();

Ticker trigger;

void triggerReading()
{
  Serial.println(dht.getStatusString());
  readSensors.SetHumidity(dht.getHumidity());
  readSensors.SetTemperature(dht.getTemperature());
}

void setup()
{
  dht.setup(D1, DHT::DHT22);
  WiFi.disconnect(true);
  delay(1000);
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
    Serial.println(eepromsettings.getSetting("SSID"));
    Serial.println(eepromsettings.getSetting("WIFIPASS"));
    WiFi.begin(eepromsettings.getSetting("SSID").c_str(), eepromsettings.getSetting("WIFIPASS").c_str());

    int failCounter = 0;
    WiFi.mode(WIFI_STA);
    while ((WiFi.status() != WL_CONNECTED) && ( failCounter < 60 ))
    {
      Serial.println(WiFi.status());
      delay(500);
      Serial.print(".");
      failCounter++;
    }

    if (failCounter == 60)
    {
      Serial.println("Failed to connect to WIFI, will create access point!");
      bWifiFailed = true;
    }
    else
    {
      Serial.println("");
    }
  }
  if ( (bWifiFailed) )
  {
    Serial.println("Setting up AP");
    // TODO: FIX IP ADDRESS SETTINGS!!!!
    //Start Access Point
    WiFi.mode(WIFI_AP); 
    bool result = WiFi.softAP("MafraLabWS", "123456789");
    IPAddress myIP = WiFi.softAPIP();
	  Serial.print("AP IP address: ");
	  Serial.println(myIP);
    if (result)
    {
      Serial.println("Access Point Started");
    }
    else
    {
      Serial.println("Failed to start Access Point");
    }
  }

  if (!SPIFFS.begin())
  {
    Serial.println("SPIFF mount failed");
  }
  else
  {
    Serial.println("SPIFF mount succeded");
  }
delay(500);
Serial.println("Starting Read stuff in SPIFFS");
Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
   
   Serial.println();
    }

    Serial.println("Ended SPIFF Reading");


const char* remote_host = "www.google.com";
serveContent=ServeContent(&server, &eepromsettings, &readSensors);
Serial.println(WiFi.localIP());
if(Ping.ping(remote_host)) 
{
  server.on("/", std::bind(&ServeContent::mainPage, serveContent));
  server.on("/login", std::bind(&ServeContent::loginPage, serveContent));
  server.on("/resetpassword", std::bind(&ServeContent::resetPassword, serveContent));
  server.on("/wifisettings", std::bind(&ServeContent::wifiSetting, serveContent));
  server.on("/reading", std::bind(&ServeContent::getReadings, serveContent));
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works without need of authentification");
  });
} 
else 
{
  Serial.println(WiFi.localIP());
  server.on("/", std::bind(&ServeContent::noInternet, serveContent));
}
  server.onNotFound(std::bind(&ServeContent::handleNotFound, serveContent));
  //here the list of headers to be recorded
  const char *headerkeys[] = {"User-Agent", "Cookie"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  //ask server to track these headers
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();
  Serial.println("HTTP server started");

  if (!MDNS.begin("weatherstation")) {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  Serial.println(dht.getMinimumSamplingPeriod());

  trigger.attach_ms(dht.getMinimumSamplingPeriod()+1500, triggerReading);
}

void loop()
{
  server.handleClient(); 
}

