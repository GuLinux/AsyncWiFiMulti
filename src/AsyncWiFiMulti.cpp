#include "AsyncWiFiMulti.h"
#include <functional>
#include <Arduino.h>

#if DEBUG_WIFI_MULTI
#define DBG(...) DEBUG_WIFI_MULTI_PORT.printf("[AsyncWiFiMulti] "); DEBUG_WIFI_MULTI_PORT.printf(__VA_ARGS__); DEBUG_WIFI_MULTI_PORT.println()
#else
#define DBG(...)
#endif

using namespace GuLinux;
using namespace std::placeholders;

AsyncWiFiMulti::AsyncWiFiMulti() {
}

AsyncWiFiMulti::~AsyncWiFiMulti() {
    WiFi.removeEvent(event_id);
}

bool AsyncWiFiMulti::addAP(const char* ssid, const char *passphrase) {
    const ApSettings newAP = {ssid, passphrase};
    DBG("Adding AP: %s", newAP.ssid.c_str());
    if(!newAP.valid()) {
        DBG("Invalid AP, skipping");
        return false;
    }
    if(std::any_of(configuredAPs.begin(), configuredAPs.end(), [&newAP](const ApSettings& ap) {
        return ap.ssid == newAP.ssid;
    })) {
        DBG("AP %s already exists, skipping", newAP.ssid.c_str());
        return false;
    }
    DBG("AP %s added successfully", newAP.ssid.c_str());
    configuredAPs.push_front(newAP);
    return true;
}

bool AsyncWiFiMulti::start() {
    if(running) {
        DBG("AsyncWiFiMulti already running");
        return false;
    }
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);

    WiFi.scanNetworks(true, true);
    event_id = WiFi.onEvent(std::bind(&AsyncWiFiMulti::onEvent, this, _1, _2));
    running = true;
    return true;
}

bool AsyncWiFiMulti::ApSettings::valid() const {
    if(ssid.empty() || ssid.length() < 1 || ssid.length() > 31) {
        return false;
    }
    if(passphrase.length() > 64) {
        return false;
    }
    return true;
}

void AsyncWiFiMulti::onEvent(arduino_event_id_t event_type, const arduino_event_info_t &event_info){
    if(!running) {
        if(event_type == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
            char ssid[33] = {0};
            memcpy(ssid, event_info.wifi_sta_disconnected.ssid, event_info.wifi_sta_disconnected.ssid_len);
            DBG("WiFi disconnected: %s, reason: %d", ssid, event_info.wifi_sta_disconnected.reason);
            if(onDisconnectedCallback) {
                onDisconnectedCallback(ssid, event_info.wifi_sta_disconnected.reason);
            }
        }
        return;
    }
    DBG("Event received: %d", event_type);
    switch (event_type) {
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
        onScanDone(event_info.wifi_scan_done);  
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        DBG("WiFi disconnected: reason=%d", event_info.wifi_sta_disconnected.reason);
        currentAp++;
        tryNextAP();
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        DBG("WiFi connected: SSID=`%s`, IP=%s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        if(onConnectedCallback) {
            onConnectedCallback(*currentAp);
        }
        running = false;
        break;
    default:
        break;
    }
    
}

void AsyncWiFiMulti::onScanDone(const wifi_event_sta_scan_done_t &scanInfo) {
    DBG("Scan done: status=%d, number=%d, scan_id=%d", scanInfo.status, scanInfo.number, scanInfo.scan_id);
    if(scanInfo.status != 0) {
        DBG("Scan failed with status %d", scanInfo.status);
        return;
    }
    
    ApSettings::List allFoundAPs;
    for(int i = 0; i < scanInfo.number; ++i) {
        ApSettings found = {
            {WiFi.SSID(i).c_str()},
            "",
            WiFi.RSSI(i),
            WiFi.channel(i),
        };
        DBG("Found AP: %s, RSSI: %d, channel: %i", found.ssid.c_str(), found.rssi, found.channel);
        allFoundAPs.push_front(found);
    }
    
    // Sort by RSSI
    allFoundAPs.sort([](const ApSettings &a, const ApSettings &b) {
        return a.rssi> b.rssi;
    });
    foundAPs.clear();
    for(auto &ap : allFoundAPs) {
        if(ap.valid() && std::none_of(foundAPs.begin(), foundAPs.end(), [&ap](const ApSettings& existing) {
            return existing.ssid == ap.ssid;
        })) {
            for(auto &configured : configuredAPs) {
                if(configured.ssid == ap.ssid) {
                    ap.passphrase = configured.passphrase; // Use passphrase from configured APs
                    foundAPs.push_back(ap);
                    break;
                }
            }
        }
    }

    #if DEBUG_WIFI_MULTI
    DBG("Sorted APs by RSSI:");
    for(const auto &ap : allFoundAPs) {
        DBG("Scan AP: [%d] %s", ap.rssi, ap.ssid.c_str());
    }
    for(const auto &ap : foundAPs) {
        DBG("Found configured AP: %s, RSSI: %d, channel: %i, passphrase: %s", ap.ssid.c_str(), ap.rssi, ap.channel, ap.passphrase.c_str());
    }
    #endif
    currentAp = foundAPs.begin();
    tryNextAP();
}

void AsyncWiFiMulti::tryNextAP() {
    if(currentAp != foundAPs.end()) {
        DBG("Trying next AP: %s", currentAp->ssid.c_str());
        WiFi.begin(currentAp->ssid.c_str(), currentAp->passphrase.c_str());
    } else {
        DBG("No more APs to try");
        if(onFailureCallback) {
            onFailureCallback();
        }
        running = false;
    }
}
