
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
#include <ESP8266HTTPClient.h>
#include <SFE_BMP180.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

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
    if (bmp.begin())
        Serial.println("BMP180 init success");
    dht.setup(D6, DHT::DHT22);
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
        while ((WiFi.status() != WL_CONNECTED) && (failCounter < 60))
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
    else
    {
        bWifiFailed=true;
    }
    if ((bWifiFailed))
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
    while (dir.next())
    {
        String fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
    }

    Serial.println("Ended SPIFF Reading");

    const char *remote_host = "www.google.com";
    serveContent = ServeContent(&server, &eepromsettings, &readSensors);
    Serial.println(WiFi.localIP());
    if (Ping.ping(remote_host))
    {
        server.on("/", std::bind(&ServeContent::mainPage, serveContent));
        server.on("/restart", std::bind(&ServeContent::restart, serveContent));
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
        server.on("/restart", std::bind(&ServeContent::restart, serveContent));
    }
    server.onNotFound(std::bind(&ServeContent::handleNotFound, serveContent));
    //here the list of headers to be recorded
    const char *headerkeys[] = {"User-Agent", "Cookie"};
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
    //ask server to track these headers
    server.collectHeaders(headerkeys, headerkeyssize);
    server.begin();
    Serial.println("HTTP server started");

    if (!MDNS.begin("weatherstation"))
    {
        Serial.println("Error setting up MDNS responder!");
        while (1)
        {
            delay(1000);
        }
    }
    Serial.println("mDNS responder started");

    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);

    Serial.println(dht.getMinimumSamplingPeriod());

    trigger.attach_ms(dht.getMinimumSamplingPeriod() + 1500, triggerReading);
    trigger.attach_ms(dht.getMinimumSamplingPeriod() + 1500, triggerReading);


  // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname("weatherstation");
    ArduinoOTA.onStart([]() {
        Serial.println("Start Firmware Update");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd Firmware Update");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop()
{
    server.handleClient();
    if (bReadSensor)
    {
        char status;
        double T, P, p0, a;
        double temperature;

        Serial.println();
        Serial.print("provided altitude: ");
        Serial.print(ALTITUDE, 0);
        Serial.print(" meters, ");

        status = bmp.startTemperature();
        if (status != 0)
        {
            // Wait for the measurement to complete:
            delay(status);

            // Retrieve the completed temperature measurement:
            // Note that the measurement is stored in the variable T.
            // Function returns 1 if successful, 0 if failure.

            status = bmp.getTemperature(T);
            if (status != 0)
            {
                // Print out the measurement:
                Serial.print("temperature: ");
                Serial.print(T, 2);
                Serial.print(" deg C, ");

                status = bmp.startPressure(3);
                if (status != 0)
                {
                    // Wait for the measurement to complete:
                    delay(status);

                    // Retrieve the completed pressure measurement:
                    // Note that the measurement is stored in the variable P.
                    // Note also that the function requires the previous temperature measurement (T).
                    // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
                    // Function returns 1 if successful, 0 if failure.
                    temperature = (T + (double)readSensors.GetTemperature()) / (double)2;
                    status = bmp.getPressure(P, temperature);
                    if (status != 0)
                    {
                        // Print out the measurement:
                        Serial.print("absolute pressure: ");
                        Serial.print(P, 2);
                        Serial.print(" mb, ");
                        Serial.print(P * 0.0295333727, 2);
                        Serial.println(" inHg");

                        // The pressure sensor returns abolute pressure, which varies with altitude.
                        // To remove the effects of altitude, use the sealevel function and your current altitude.
                        // This number is commonly used in weather reports.
                        // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
                        // Result: p0 = sea-level compensated pressure in mb
                        p0 = bmp.sealevel(P, ALTITUDE);
                        Serial.print("relative (sea-level) pressure: ");
                        Serial.print(p0, 2);
                        Serial.print(" mb, ");
                        Serial.print(p0 * 0.0295333727, 2);
                        Serial.println(" inHg");
                    }
                    else
                        Serial.println("error retrieving pressure measurement\n");
                }
                else
                    Serial.println("error starting pressure measurement\n");
            }
            else
                Serial.println("error retrieving temperature measurement\n");
        }
        else
            Serial.println("error starting temperature measurement\n");

        readSensors.SetHumidity(dht.getHumidity());
        readSensors.SetTemperature((float)temperature);
        readSensors.SetPressure((float)p0);

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
            Serial.print("[HTTP] POST... code:");
            Serial.println(httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK)
            {
                String payload = http.getString();
                Serial.println(payload);
            }
        }
        else
        {
            Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.writeToStream(&Serial);
        Serial.println("");
        http.end();

        bReadSensor = false;
    }
    ArduinoOTA.handle();
}
