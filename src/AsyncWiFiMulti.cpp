#include "AsyncWiFiMulti.h"
#include <functional>
#include <Arduino.h>

#define __DBG(level, ...) DEBUG_WIFI_MULTI_PORT.printf("[" level "][AsyncWiFiMulti] "); DEBUG_WIFI_MULTI_PORT.printf(__VA_ARGS__); DEBUG_WIFI_MULTI_PORT.println()

#if DEBUG_WIFI_MULTI >= WIFI_MULTI_LOGLEVEL_DEBUG
#define lDebug(...) __DBG("DEBUG", __VA_ARGS__)
#endif
#if DEBUG_WIFI_MULTI >= WIFI_MULTI_LOGLEVEL_VERBOSE
#define lVerbose(...) __DBG("VERBOSE", __VA_ARGS__)
#endif
#if DEBUG_WIFI_MULTI >= WIFI_MULTI_LOGLEVEL_INFO
#define lInfo(...) __DBG("INFO", __VA_ARGS__)
#endif

#ifndef lDebug
#define lDebug(...)
#endif
#ifndef lVerbose
#define lVerbose(...)
#endif
#ifndef lInfo
#define lInfo(...)
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
    lVerbose("Adding AP: %s", newAP.ssid.c_str());
    if(!newAP.valid()) {
        lInfo("Invalid AP, skipping");
        return false;
    }
    if(std::any_of(configuredAPs.begin(), configuredAPs.end(), [&newAP](const ApSettings& ap) {
        return ap.ssid == newAP.ssid;
    })) {
        lInfo("AP %s already exists, skipping", newAP.ssid.c_str());
        return false;
    }
    lVerbose("AP %s added successfully", newAP.ssid.c_str());
    configuredAPs.push_front(newAP);
    return true;
}

bool AsyncWiFiMulti::start() {
    if(running) {
        lInfo("AsyncWiFiMulti already running");
        return false;
    }
    WiFi.disconnect(false, false);
    delay(10); // Ensure WiFi is disconnected before starting
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
            lInfo("WiFi disconnected: %s, reason: %s", ssid, WiFi.disconnectReasonName(static_cast<wifi_err_reason_t>(event_info.wifi_sta_disconnected.reason)));
            if(onDisconnectedCallback) {
                onDisconnectedCallback(ssid, event_info.wifi_sta_disconnected.reason);
            }
        }
        if(event_type == ARDUINO_EVENT_WIFI_STA_LOST_IP) {
            lInfo("WiFi: lost IP event received");
            if(onDisconnectedCallback) {
                onDisconnectedCallback(WiFi.SSID().c_str(), -1);
            }
        }
        return;
    }
    lDebug("Event received: %s", WiFi.eventName(event_type));
    switch (event_type) {
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
        onScanDone(event_info.wifi_scan_done);  
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        lVerbose("WiFi disconnected: reason=%d", event_info.wifi_sta_disconnected.reason);
        currentAp++;
        tryNextAP();
        break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
    lVerbose("WiFi lost IP: SSID=%s", WiFi.SSID().c_str());
        currentAp++;
        tryNextAP();
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        lInfo("WiFi connected: SSID=`%s`, IP=%s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
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
    lVerbose("Scan done: status=%d, number=%d, scan_id=%d", scanInfo.status, scanInfo.number, scanInfo.scan_id);
    if(scanInfo.status != 0) {
        lInfo("Scan failed with status %d", scanInfo.status);
        onFailure();
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
        lDebug("Found AP: %s, RSSI: %d, channel: %i", found.ssid.c_str(), found.rssi, found.channel);
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
    lDebug("Sorted APs by RSSI:");
    for(const auto &ap : allFoundAPs) {
        lDebug("Scan AP: [%d] %s", ap.rssi, ap.ssid.c_str());
    }
    for(const auto &ap : foundAPs) {
        lDebug("Found configured AP: %s, RSSI: %d, channel: %i, passphrase: %s", ap.ssid.c_str(), ap.rssi, ap.channel, ap.passphrase.c_str());
    }
    #endif
    currentAp = foundAPs.begin();
    tryNextAP();
}

void GuLinux::AsyncWiFiMulti::onFailure() {
    lInfo("Failed to connect to any configured APs");
    running = false;
    if(onFailureCallback) {
        onFailureCallback();
    }
}

void AsyncWiFiMulti::tryNextAP() {
    if(currentAp != foundAPs.end()) {
        lVerbose("Trying next AP: %s", currentAp->ssid.c_str());
        WiFi.begin(currentAp->ssid.c_str(), currentAp->passphrase.c_str());
    } else {
        lVerbose("No more APs to try");
        onFailure();
    }
}
