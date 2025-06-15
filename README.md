# AsyncWiFiMulti

This is a small library for the ESP32 platform, with the intent of replacing the [WiFiMulti](https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiMulti.h) class with an asynchronous verson.


## Installation

### PlatformIO

If using PlatformIO, simply use the library manager, or add

```
gulinux/AsyncWiFiMulti@^0.1.0
```
to the `lib_deps` section of your project.

[![PlatformIO Registry](https://badges.registry.platformio.org/packages/gulinux/library/AsyncWiFiMulti.svg)](https://registry.platformio.org/libraries/gulinux/AsyncWiFiMulti)

### Arduino

You can just install this library using the builtin library manager, or by going to the [releases](https://github.com/GuLinux/AsyncWiFiMulti/releases) page, downloading the zip fileof the version you want to install, and unzipping it in your `Arduino/libraries` folder. See the [arduino docs](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries/) for more details.

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

## Examples/tests

Please look at the [test file](https://github.com/GuLinux/AsyncWiFiMulti/blob/main/test/main.cpp) for a working example on how to use the class.

You can also launch tests using platformio using the `pio test` command. You only need to provide an `aps_configuration.h` file filled with your APs credentials.



`onConnected` and `onFailure` will only be called if `AsyncWiFiMulti` is currently in `running` state. `running` will be set to `true` after calling `start`, 
and will be set to `false` after either a successful connection, or a failure.

