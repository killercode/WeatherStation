#include "Arduino.h"
#include "servecontent.h"
#include <ESP8266WebServer.h>
#include "eeprommanager.h"
#include "FS.h"

ESP8266WebServer *myserver;
EEPROMManager *settingManager;
Reading *readSensor;

ServeContent::ServeContent()
{
}

ServeContent::ServeContent(ESP8266WebServer *pServer, EEPROMManager *eeprommanager, Reading *reading)
{
  myserver = pServer;
  settingManager = eeprommanager;
  readSensor = reading;
}

//Check if header is present and correct
bool ServeContent::is_authenticated()
{
  Serial.println("Enter is_authentified");
  if (myserver->hasHeader("Cookie"))
  {
    Serial.print("Found cookie: ");
    String cookie = myserver->header("Cookie");
    Serial.println(cookie);
    if (cookie.indexOf("ESPSESSIONID=1") != -1)
    {
      Serial.println("Authentification Successful");
      return true;
    }
  }
  Serial.println("Authentification Failed");
  return false;
}

//login page, also called for disconnect
void ServeContent::loginPage()
{
  String msg;
  if (myserver->hasHeader("Cookie"))
  {
    Serial.print("Found cookie: ");
    String cookie = myserver->header("Cookie");
    Serial.println(cookie);
  }
  if (myserver->hasArg("DISCONNECT"))
  {
    Serial.println("Disconnection");
    String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    myserver->sendContent(header);
    return;
  }
  if (myserver->hasArg("USERNAME") && myserver->hasArg("PASSWORD"))
  {
    Serial.println("STORED USERNAME: " + settingManager->getSetting("USERNAME"));
    Serial.println("STORED PASSWORD: " + settingManager->getSetting("USERPASS"));
    if (myserver->arg("USERNAME").equals(settingManager->getSetting("USERNAME")) && myserver->arg("PASSWORD").equals(settingManager->getSetting("USERPASS")))
    {
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=1\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      myserver->sendContent(header);
      Serial.println("Log in Successful");
      return;
    }
    String header = "HTTP/1.1 401 OK\r\nCache-Control: no-cache\r\n\r\n";
    myserver->sendContent(header);
    Serial.println("Failed Login");
    return;
  }

  if (SPIFFS.exists("/login.html"))
  {
    File file = SPIFFS.open("/login.html", "r");
    myserver->streamFile(file, "text/html");
    file.close();
  }
}

void ServeContent::noInternet()
{
  Serial.println("Enter noInternet");
  String header;
  String msg = "";
  
  if (myserver->hasArg("SSID") && myserver->hasArg("WIFIPASS"))
  {
    if (myserver->arg("SSID").length() > 1)
    {
      settingManager->setSetting("SSID", myserver->arg("SSID"));
    }
    else
    {
      msg += "SSID is too short<br> ";
    }
    if (myserver->arg("WIFIPASS").length() > 1)
    {
      settingManager->setSetting("WIFIPASS", myserver->arg("WIFIPASS"));
    }
    else
    {
      msg += "Wifi password too short";
    }
    if (msg.length() == 0)
    {
      settingManager->writeRecords();
      settingManager->settings.clear();
      settingManager->readRecords();
      WiFi.disconnect(true);
      ESP.restart();
      delay(1000);
    }
  }

  if (SPIFFS.exists("/nointernet.html"))
  {
    File file = SPIFFS.open("/nointernet.html", "r");
    myserver->streamFile(file, "text/html");
    file.close();
  }
}

//root page can be accessed only if authentification is ok
void ServeContent::mainPage()
{
  // TODO: REPLACE ALL LOGIC TO WORK WITH AJAX!!!!!!!!!!!!!!!!!!!!!!!!!!
  Serial.println("Enter handleRoot");
  String header;
  /*if (!is_authenticated())
  {
    String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    myserver->sendContent(header);
    return;
  }*/
  if (SPIFFS.exists("/main.html"))
  {
    File file = SPIFFS.open("/main.html", "r");
    myserver->streamFile(file, "text/html");
    file.close();
  }
}


void ServeContent::getReadings()
{
  Serial.println("Enter getReadings");


  // TODO Implement data provider
  myserver->send(200, "text/html", "{\"temperature\": " + String(readSensor->GetTemperature()) +  ",\"humidity\":" + String(readSensor->GetHumidity()) + "}");
}

//root page can be accessed only if authentification is ok
void ServeContent::resetPassword()
{
  // TODO: REPLACE ALL LOGIC TO WORK WITH AJAX!!!!!!!!!!!!!!!!!!!!!!!!!!
  Serial.println("Enter handleResetPassword");
  String header;
  String msg = "";
  if (!is_authenticated())
  {
    String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    myserver->sendContent(header);
    return;
  }

  if (myserver->hasArg("OLDPASSWORD") && myserver->hasArg("NEWPASSWORD"))
  {
    String oldpassword = settingManager->getSetting("USERPASS");
    if (oldpassword.equals(myserver->arg("OLDPASSWORD")))
    {
      if (!myserver->arg("NEWPASSWORD").equals(""))
      {
        Serial.print("OLD PASSWORD: ");
        Serial.println(settingManager->getSetting("USERPASS"));
        Serial.print("NEW PASSWORD: ");
        Serial.println(myserver->arg("NEWPASSWORD"));
        settingManager->setSetting("USERPASS", myserver->arg("NEWPASSWORD"));
        settingManager->writeRecords();
        settingManager->settings.clear();
        settingManager->readRecords();
        Serial.print("NEW STORED PASSWORD: ");
        Serial.println(settingManager->getSetting("USERPASS"));

        for (int i = 0; i < settingManager->settings.size(); i++)
        {
          Serial.print("KEY: ");
          Serial.println(settingManager->settings.get(i).getKey());
          Serial.print("VALUE: ");
          Serial.println(settingManager->settings.get(i).getValue());
        }
        Serial.println("SETTINGS SAVE!");
      }
      else
      {
        msg = "Password field is empty";
      }
    }
    else
    {
      msg = "Old password is incorrect";
    }
  }

  String content = "<html><body><form action='/resetpassword' method='POST'>";
  content += "Change your password:<br/>";
  content += "<input type='password' name='OLDPASSWORD' placeholder='old password'><br/>";
  content += "<input type='password' name='NEWPASSWORD' placeholder='new password'><br/>";
  content += "<input type='submit' name='SUBMIT' value='Change Password'>";
  content += "</form>" + msg;
  content += "</body></html>";
  myserver->send(200, "text/html", content);
}

//root page can be accessed only if authentification is ok
void ServeContent::wifiSetting()
{
  // TODO: REPLACE ALL LOGIC TO WORK WITH AJAX!!!!!!!!!!!!!!!!!!!!!!!!!!
  Serial.println("Enter handleConfigureWifi");
  String header;
  String msg = "";
  if (!is_authenticated())
  {
    String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    myserver->sendContent(header);
    return;
  }

  if (myserver->hasArg("SSID") && myserver->hasArg("WIFIPASS"))
  {
    if (myserver->arg("SSID").length() > 1)
    {
      settingManager->setSetting("SSID", myserver->arg("SSID"));
    }
    else
    {
      msg += "SSID is too short<br> ";
    }
    if (myserver->arg("WIFIPASS").length() > 1)
    {
      settingManager->setSetting("WIFIPASS", myserver->arg("WIFIPASS"));
    }
    else
    {
      msg += "Wifi password too short";
    }
    if (msg.length() == 0)
    {
      settingManager->writeRecords();
      settingManager->settings.clear();
      settingManager->readRecords();
    }
  }

  String content = "<html><body>";
  content += "<form action='/wifisettings' method='POST'>";
  content += "Configure your wifi connection:<br/>";
  content += "<input type='input' name='SSID' placeholder='SSID' value='" + settingManager->getSetting("SSID") + "'> <br/>";
  content += "<input type='password' name='WIFIPASS' placeholder='Wifi Password' value='" + settingManager->getSetting("WIFIPASS") + "'><br/>";
  content += "<input type='submit' name='SUBMIT' value='Change'>";
  content += "</form>" + msg + "<br/>";
  content += "</body></html>";
  myserver->send(200, "text/html", content);
}

//no need authentification
void ServeContent::handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += myserver->uri();
  message += "\nMethod: ";
  message += (myserver->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += myserver->args();
  message += "\n";
  for (uint8_t i = 0; i < myserver->args(); i++)
  {
    message += " " + myserver->argName(i) + ": " + myserver->arg(i) + "\n";
  }
  myserver->send(404, "text/plain", message);
}
