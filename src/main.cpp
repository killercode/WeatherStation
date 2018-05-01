/*
 * Blink
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */

#include "Arduino.h"
#include "eeprommanager.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#ifndef UNIT_TEST // IMPORTANT LINE!

ESP8266WebServer server(80);
EEPROMManager eepromsettings = EEPROMManager();

//Check if header is present and correct
bool is_authentified()
{
  Serial.println("Enter is_authentified");
  if (server.hasHeader("Cookie"))
  {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
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
void handleLogin()
{
  String msg;
  if (server.hasHeader("Cookie"))
  {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
  }
  if (server.hasArg("DISCONNECT"))
  {
    Serial.println("Disconnection");
    String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }
  if (server.hasArg("USERNAME") && server.hasArg("PASSWORD"))
  {
    Serial.println("STORED USERNAME: " + eepromsettings.getSetting("USERNAME"));
    Serial.println("STORED PASSWORD: " + eepromsettings.getSetting("USERPASS"));
    if (server.arg("USERNAME").equals(eepromsettings.getSetting("USERNAME" )) && server.arg("PASSWORD").equals( eepromsettings.getSetting("USERPASS")))
    {
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=1\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      Serial.println("Log in Successful");
      return;
    }
    msg = "Wrong username/password! try again.";
    Serial.println("Log in Failed");
  }
  String content = "<html><body><form action='/login' method='POST'>To log in, please use : admin/admin<br>";
  content += "User:<input type='text' name='USERNAME' placeholder='user name'><br>";
  content += "Password:<input type='password' name='PASSWORD' placeholder='password'><br>";
  content += "<input type='submit' name='SUBMIT' value='Submit'></form>" + msg + "<br>";
  content += "You also can go <a href='/inline'>here</a></body></html>";
  server.send(200, "text/html", content);
}

//root page can be accessed only if authentification is ok
void handleRoot()
{
  Serial.println("Enter handleRoot");
  String header;
  if (!is_authentified())
  {
    String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }
  String content = "<html><body><H2>hello, you successfully connected to esp8266!</H2><br>";
  if (server.hasHeader("User-Agent"))
  {
    content += "the user agent used is : " + server.header("User-Agent") + "<br><br>";
  }
  content += "You can access this page until you <a href=\"/login?DISCONNECT=YES\">disconnect</a></body></html>";
  server.send(200, "text/html", content);
}

//root page can be accessed only if authentification is ok
void handleResetPassword()
{
  Serial.println("Enter handleResetPassword");
  String header;
  String msg = "";
  if (!is_authentified())
  {
    String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }

  if (server.hasArg("OLDPASSWORD") && server.hasArg("NEWPASSWORD"))
  {
      String oldpassword = eepromsettings.getSetting("USERPASS");
      if (oldpassword.equals(server.arg("OLDPASSWORD")))
      {
          if (!server.arg("NEWPASSWORD").equals(""))
          {
            Serial.print("OLD PASSWORD: ");
            Serial.println(eepromsettings.getSetting("USERPASS"));
            Serial.print("NEW PASSWORD: ");
            Serial.println(server.arg("NEWPASSWORD"));
            eepromsettings.setSetting("USERPASS", server.arg("NEWPASSWORD"));
            eepromsettings.writeRecords();
            eepromsettings.settings.clear();
            eepromsettings.readRecords();
            Serial.print("NEW STORED PASSWORD: ");
            Serial.println(eepromsettings.getSetting("USERPASS"));

            for (int i = 0; i < eepromsettings.settings.size(); i++)
            {
              Serial.print("KEY: ");
              Serial.println(eepromsettings.settings.get(i).getKey());
              Serial.print("VALUE: ");
              Serial.println(eepromsettings.settings.get(i).getValue());
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
  server.send(200, "text/html", content);
}

//root page can be accessed only if authentification is ok
void handleConfigureWifi()
{
  Serial.println("Enter handleConfigureWifi");
  String header;
  String msg = "";
  if (!is_authentified())
  {
    String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }

  if (server.hasArg("SSID") && server.hasArg("WIFIPASS"))
  {
      if (server.arg("SSID").length() > 1)
      {
        eepromsettings.setSetting("SSID", server.arg("SSID"));
      }
      else
      {
        msg += "SSID is too short<br> ";
      }
      if (server.arg("WIFIPASS").length() > 1)
      {
        eepromsettings.setSetting("WIFIPASS", server.arg("WIFIPASS"));
      }
      else
      {
       msg += "Wifi password too short";
      }
      if (msg.length() == 0)
      {
        eepromsettings.writeRecords();
        eepromsettings.settings.clear();
        eepromsettings.readRecords();
      }
  }

  String content = "<html><body>";
        content += "<form action='/wifisettings' method='POST'>";
        content += "Configure your wifi connection:<br/>";
        content += "<input type='input' name='SSID' placeholder='SSID' value='" + eepromsettings.getSetting("SSID") + "'> <br/>";
        content += "<input type='password' name='WIFIPASS' placeholder='Wifi Password' value='" + eepromsettings.getSetting("WIFIPASS") + "'><br/>";
        content += "<input type='submit' name='SUBMIT' value='Change'>";
        content += "</form>"+msg+"<br/>";
        content += "</body></html>";
  server.send(200, "text/html", content);
}

//no need authentification
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

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

  Serial.println(WiFi.localIP());
  server.on("/", handleRoot);
  server.on("/login", handleLogin);
  server.on("/resetpassword", handleResetPassword);
  server.on("/wifisettings", handleConfigureWifi);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works without need of authentification");
  });

  server.onNotFound(handleNotFound);
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

#endif // IMPORTANT LINE!