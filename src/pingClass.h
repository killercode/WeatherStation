#include <Arduino.h>
#include <ESP8266WiFi.h>

#ifndef pingClass_h
#define pingClass_h

extern "C" {
  #include <ping.h>
}

class PingClass {
  public:
    PingClass();

    bool ping(IPAddress dest,   byte count = 5);
    bool ping(const char* host, byte count = 5);

    int averageTime();

  protected:
    static void _ping_sent_cb(void *opt, void *pdata);
    static void _ping_recv_cb(void *opt, void *pdata);

    IPAddress _dest;
    ping_option _options;

    static byte _expected_count, _errors, _success;
    static int _avg_time;
};

#endif