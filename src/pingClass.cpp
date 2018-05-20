#include "pingClass.h"

extern "C" void esp_schedule();
extern "C" void esp_yield();


PingClass::PingClass() 
{
}

bool PingClass::ping(IPAddress dest, byte count) {
    _expected_count = count;
    _errors = 0;
    _success = 0;

    _avg_time = 0;

    memset(&_options, 0, sizeof(struct ping_option));

    // Repeat count (how many time send a ping message to destination)
    _options.count = count;
    // Time interval between two ping (seconds??)
    _options.coarse_time = 1;
    // Destination machine
    _options.ip = dest;

    // Callbacks
    _options.recv_function = reinterpret_cast<ping_recv_function>(&PingClass::_ping_recv_cb);
    _options.sent_function = NULL;  // reinterpret_cast<ping_sent_function>(&_ping_sent_cb);

    // Let's go!
    if(ping_start(&_options)) {
        // Suspend till the process end
        esp_yield();
    }

    return (_success > 0);
}

bool PingClass::ping(const char* host, byte count) {
    IPAddress remote_addr;

    if (WiFi.hostByName(host, remote_addr))
        return ping(remote_addr, count);

    return false;
}

void PingClass::_ping_recv_cb(void *opt, void *resp) {
    // Cast the parameters to get some usable info
    ping_resp*   ping_resp = reinterpret_cast<struct ping_resp*>(resp);
    // ping_option* ping_opt  = reinterpret_cast<struct ping_option*>(opt);

    // Error or success?
    if (ping_resp->ping_err == -1)
    {
         _errors++;
    }   
    else 
    {
        _success++;
        _avg_time += ping_resp->resp_time;
    }

    // Is it time to end?
    // Don't using seqno because it does not increase on error
    if (_success + _errors == _expected_count) {
        _avg_time = _success > 0 ? _avg_time / _success : 0;

        // Done, return to main functiom
        esp_schedule();
    }
}

byte PingClass::_expected_count = 0;
byte PingClass::_errors = 0;
byte PingClass::_success = 0;
int  PingClass::_avg_time = 0;
