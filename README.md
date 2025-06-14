# AsyncWiFiMulti

This is a small library for the ESP32 platform, with the intent of replacing the [WiFiMulti](https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiMulti.h) class with an asynchronous verson.

## API

```
    bool addAP(const char* ssid, const char *passphrase = nullptr);
```
Adds an access point to the configuration. Returns `true` if adding was successful, `false` if the AP SSID/passphrase are not valid, or if the AP is already present.

```
    bool start();
```

This is  the main entry point for starting `AsyncWiFiMulti`. Returns `true` if startup was successful, `false` if already running.
AsyncWiFiMulti will:
 - Switch to `running` state.
 - Scan for available access points.
 - Filter for the configured access points, and sort them by signal strenght.
 - Try to connect to access points sequentially, from the strongest to the weakest, moving to the next when connection fails.
 - If any connection was successful:
   - Call `OnConnected` if the callback was set
   - set `running` state to `false`.
 - If no connection was successful:
   - Call `OnDisconnected` if the callback was set
   - set `running` state to `false`.

`OnConnected` and `OnFailure` will *only* be called when `running` state is `true`.
There is a third callback available `OnDisconnected`. This will *only* be called if `running` is false, and will allow you to implemet your reconnection logic if necessary.

`AsyncWiFiMulti` has no retry policy by default, but you can easily use the `OnDisconnected` and `OnFailure` callbacks to call the `start` method again.

### Callbacks signature

```
    using OnConnected = std::function<void(const ApSettings&)>;
    using OnFailure = std::function<void()>;
    using OnDisconnected = std::function<void(const char *ssid, uint8_t disconnectionReason)>;
```

### Callbacks setters

```
    void onConnected(OnConnected callback);
    void onFailure(OnFailure callback);
    void onDisconnected(OnDisconnected callback);
```


`onConnected` and `onFailure` will only be called if `AsyncWiFiMulti` is currently in `running` state. `running` will be set to `true` after calling `start`, 
and will be set to `false` after either a successful connection, or a failure.

