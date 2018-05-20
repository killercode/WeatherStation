#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <ESP8266HTTPClient.h>
#include <SFE_BMP180.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "./reading.h"
#include "./eeprommanager.h"
#include "./servecontent.h"
#include "./pingClass.h"
#include "DHT.h"
#ifdef DEBUG
    #define DEBUG_PRINTLN(x)  Serial.println (x)
    #define DEBUG_PRINT(x) Serial.print(x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
#endif

#define ALTITUDE 229.0


DHT dht;
ESP8266WebServer server(80);
EEPROMManager eepromsettings = EEPROMManager();
ServeContent serveContent;
PingClass Ping = PingClass();
Reading readSensors = Reading();
SFE_BMP180 bmp;
Ticker trigger;

bool bReadSensor = false;

void triggerReading()
{
    bReadSensor = true;
}

void setup()
{
    bool bWifiFailed = false;

    Serial.begin(115200);
    DEBUG_PRINTLN("Starting...");

    // Initializing the BMP Library
    if (bmp.begin())
    {
        DEBUG_PRINTLN("BMP180 init success");
    }

    // Initializing the DHT Library
    dht.setup(D6, DHT::DHT22);

    // Make sure we clean all the wireless settings
    WiFi.disconnect(true);

    // Read stored EEPROM settings
    eepromsettings.readRecords();

    DEBUG_PRINTLN("Trying to fetch wifi settings from EEPROM");
    if (eepromsettings.hasSetting("SSID") &&
        eepromsettings.hasSetting("WIFIPASS"))
    {
        // Start WIFI
        DEBUG_PRINTLN(eepromsettings.getSetting("SSID"));
        DEBUG_PRINTLN(eepromsettings.getSetting("WIFIPASS"));

        WiFi.begin(eepromsettings.getSetting("SSID").c_str(),
                    eepromsettings.getSetting("WIFIPASS").c_str());

        int failCounter = 0;

        WiFi.mode(WIFI_STA);
        while ((WiFi.status() != WL_CONNECTED) && (failCounter < 60))
        {
            DEBUG_PRINTLN(WiFi.status());
            delay(500);
            DEBUG_PRINT(".");
            failCounter++;
        }

        if (failCounter == 60)
        {
            DEBUG_PRINTLN("Failed to connect to WIFI, will create access point!");
            bWifiFailed = true;
        }
    }
    else
    {
        DEBUG_PRINTLN("No settings stored on EEPROM...")
        bWifiFailed = true;
    }
    if ((bWifiFailed))
    {
        DEBUG_PRINTLN("Setting up AP");

        // Start Access Point
        WiFi.mode(WIFI_AP);
        bool result = WiFi.softAP("MafraLabWS", "123456789");
        IPAddress myIP = WiFi.softAPIP();
        DEBUG_PRINT("AP IP address: ");
        DEBUG_PRINTLN(myIP);
        if (result)
        {
            DEBUG_PRINTLN("Access Point Started");
        }
        else
        {
            DEBUG_PRINTLN("Failed to start Access Point");
            DEBUG_PRINTLN("Restart");
            ESP.restart();
        }
    }

    if (!SPIFFS.begin())
    {
        DEBUG_PRINTLN("SPIFF mount failed");
    }
    else
    {
        DEBUG_PRINTLN("SPIFF mount succeded");
    }

    DEBUG_PRINTLN("Starting Read stuff in SPIFFS");
    Dir dir = SPIFFS.openDir("/");
    while (dir.next())
    {
        String fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        DEBUG_PRINTLN("FS File: "+ fileName +", size: " + String(fileSize));
    }
    DEBUG_PRINTLN("Ended SPIFF Reading");

    // check internet connectivity
    const char *remote_host = "www.google.com";
    serveContent = ServeContent(&server, &eepromsettings, &readSensors);

    if (Ping.ping(remote_host))
    {
        server.on("/", std::bind(&ServeContent::mainPage, serveContent));
        server.on("/restart", std::bind(&ServeContent::restart, serveContent));
        server.on("/login", std::bind(&ServeContent::loginPage, serveContent));
        server.on("/resetpassword", std::bind(&ServeContent::resetPassword, serveContent));
        server.on("/wifisettings", std::bind(&ServeContent::wifiSetting, serveContent));
        server.on("/reading", std::bind(&ServeContent::getReadings, serveContent));
    }
    else
    {
        server.on("/", std::bind(&ServeContent::noInternet, serveContent));
        server.on("/restart", std::bind(&ServeContent::restart, serveContent));
    }
    server.onNotFound(std::bind(&ServeContent::handleNotFound, serveContent));
    
    // here the list of headers to be recorded
    const char *headerkeys[] = {"User-Agent", "Cookie"};
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
    // ask server to track these headers
    server.collectHeaders(headerkeys, headerkeyssize);
    server.begin();
    
    DEBUG_PRINTLN("HTTP server started");


    // Setting up mDNS
    if (!MDNS.begin("weatherstation"))
    {
        DEBUG_PRINTLN("Error setting up MDNS responder!");
        while (1)
        {
            delay(1000);
        }
    }
    DEBUG_PRINTLN("mDNS responder started");

    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);

    trigger.attach_ms(15000, triggerReading);

    // Hostname defaults to weatherstation
    ArduinoOTA.setHostname("weatherstation");
    ArduinoOTA.onStart([]() 
    {
        DEBUG_PRINTLN("Start Firmware Update");
    });

    ArduinoOTA.onEnd([]() 
    {
        DEBUG_PRINTLN("\nEnd Firmware Update");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
    {
        DEBUG_PRINTLN("Progress: " + String(progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) 
    {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
        {
            DEBUG_PRINTLN("Auth Failed");
        }
        else if (error == OTA_BEGIN_ERROR)
        {
            DEBUG_PRINTLN("Begin Failed");
        }
        else if (error == OTA_CONNECT_ERROR)
        {
            DEBUG_PRINTLN("Connect Failed");
        }
        else if (error == OTA_RECEIVE_ERROR)
        {
            DEBUG_PRINTLN("Receive Failed");
        }
        else if (error == OTA_END_ERROR)
        {
            DEBUG_PRINTLN("End Failed");
        }
    });

    ArduinoOTA.begin();
    DEBUG_PRINTLN("Ready");
}

void loop()
{
    if (bReadSensor)
    {
        char status;
        double p0 = 0;
        double temperature = 0;

        status = bmp.startTemperature();
        if (status != 0)
        {
            double T = 0;
            // Wait for the measurement to complete:
            delay(status);

            // Retrieve the completed temperature measurement:
            // Note that the measurement is stored in the variable T.
            // Function returns 1 if successful, 0 if failure.

            status = bmp.getTemperature(T);
            if (status != 0)
            {
                // Print out the measurement:
                DEBUG_PRINT("temperature: ");
                DEBUG_PRINT(T);
                DEBUG_PRINTLN(" deg C, ");

                status = bmp.startPressure(3);
                if (status != 0)
                {
                    double P = 0;
                    // Wait for the measurement to complete:
                    delay(status);

                    // Retrieve the completed pressure measurement:
                    // Note that the measurement is stored in the variable P.
                    // Note also that the function requires the previous temperature measurement (T).
                    // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
                    // Function returns 1 if successful, 0 if failure.
                    temperature = (T + static_cast<double>(readSensors.GetTemperature() ) ) / static_cast<double>(2);
                    status = bmp.getPressure(P, temperature);
                    if (status != 0)
                    {
                        // Print out the measurement:
                        DEBUG_PRINT("absolute pressure: ");
                        DEBUG_PRINT(P);
                        DEBUG_PRINT(" mb, ");

                        // The pressure sensor returns abolute pressure, which varies with altitude.
                        // To remove the effects of altitude, use the sealevel function and your current altitude.
                        // This number is commonly used in weather reports.
                        // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
                        // Result: p0 = sea-level compensated pressure in mb
                        p0 = bmp.sealevel(P, ALTITUDE);
                        DEBUG_PRINT("relative (sea-level) pressure: ");
                        DEBUG_PRINT(p0);
                        DEBUG_PRINTLN(" mb, ");
                    }
                    else
                        DEBUG_PRINTLN("error retrieving pressure measurement\n");
                }
                else
                    DEBUG_PRINTLN("error starting pressure measurement\n");
            }
            else
                DEBUG_PRINTLN("error retrieving temperature measurement\n");
        }
        else
            DEBUG_PRINTLN("error starting temperature measurement\n");

        readSensors.SetHumidity(dht.getHumidity());
        readSensors.SetTemperature(static_cast<float>(temperature));
        readSensors.SetPressure(static_cast<double>(p0));

        HTTPClient http;
        Serial.setDebugOutput(true);
        http.setReuse(true);
        http.begin("http://iot.mafralab.com:8080/api/v1/R76qsT99OjcuOITx0eZS/telemetry/"); //HTTP

        http.addHeader("Content-Type", "application/json");

        // start connection and send HTTP header
        String content = "{\"temperature\":" + String(readSensors.GetTemperature()) + ", \"humidity\":" + String(readSensors.GetHumidity()) + ", \"pressure\":" + String(readSensors.GetPressure()) + "}";
        int httpCode = http.POST(content);

        // httpCode will be negative on error
        if (httpCode > 0)
        {
            // HTTP header has been send and Server response header has been handled
            DEBUG_PRINT("[HTTP] POST... code:");
            DEBUG_PRINTLN(httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK)
            {
                String payload = http.getString();
                DEBUG_PRINTLN(payload);
            }
        }
        else
        {
            Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();

        bReadSensor = false;
    }
    server.handleClient();
    ArduinoOTA.handle();
}
