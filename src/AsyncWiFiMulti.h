#ifndef GULINUX_ASYNC_WIFI_MULTI_H
#define GULINUX_ASYNC_WIFI_MULTI_H

#include <WiFi.h>
#include <stdint.h>
#include <list>

#define WIFI_MULTI_LOGLEVEL_DEBUG   3
#define WIFI_MULTI_LOGLEVEL_VERBOSE 2
#define WIFI_MULTI_LOGLEVEL_INFO    1
#define WIFI_MULTI_LOGLEVEL_NONE    0



#ifndef DEBUG_WIFI_MULTI
#define DEBUG_WIFI_MULTI WIFI_MULTI_LOGLEVEL_NONE
#endif

#ifndef DEBUG_WIFI_MULTI_PORT
#define DEBUG_WIFI_MULTI_PORT Serial
#endif

namespace GuLinux {

class AsyncWiFiMulti
{
public:
    AsyncWiFiMulti();
    ~AsyncWiFiMulti();
    enum Status { Idle, Running, Connected };

    bool addAP(const char* ssid, const char *passphrase = nullptr);
    bool start();
    bool rescan();

    struct ApSettings {
        std::string ssid;
        std::string passphrase;
        int32_t rssi = 0;
        int32_t channel = 0;
        bool valid() const;
        using List = std::list<ApSettings>;
    };
 
    const ApSettings::List& getConfiguredAPs() const { return configuredAPs; }
    const ApSettings::List& getFoundAPs() const { return foundAPs; } 

    using OnConnected = std::function<void(const ApSettings&)>;
    using OnFailure = std::function<void()>;
    using OnDisconnected = std::function<void(const char *ssid, uint8_t disconnectionReason)>;

    void onConnected(OnConnected callback) {
        onConnectedCallback = callback;
    }
    void onFailure(OnFailure callback) {
        onFailureCallback = callback;
    }
    void onDisconnected(OnDisconnected callback) {
        onDisconnectedCallback = callback;
    }
    void clear();

    Status status() const { return _status; }
    const char *statusString();

private:
    Status _status = Idle;
    ApSettings::List configuredAPs;
    ApSettings::List foundAPs;
    ApSettings::List::iterator currentAp;

    OnConnected onConnectedCallback;
    OnFailure onFailureCallback;
    OnDisconnected onDisconnectedCallback;

    void onEvent(arduino_event_id_t, const arduino_event_info_t&);
    void onScanDone(const wifi_event_sta_scan_done_t &scanInfo);
    void onFailure();
    void tryNextAP();
    
    wifi_event_id_t event_id = 0;
};
}
#endif