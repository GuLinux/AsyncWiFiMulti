#include <AsyncWiFiMulti.h>
using namespace GuLinux;

AsyncWiFiMulti wifiMulti;
void setup() {
  wifiMulti.addAP("My_SSID", "MyPassphrase");
  wifiMulti.addAP("My_Second_SSID", "MyPassphrase");
  wifiMulti.onConnected([](const AsyncWiFiMulti::ApSettings &ap) {
    Serial.printf("Connected to AP: %s, Passphrase: %s\n", ap.ssid.c_str(), ap.passphrase.c_str());
  });
  wifiMulti.onFailure([]() {
    Serial.println("Failed to connect to any configured APs. Retrying in 2 seconds...");
    delay(2000);
    wifiMulti.start(); // Restart the connection attempt
  });
  wifiMulti.onDisconnected([](const char *ssid, uint8_t disconnectionReason) {
    Serial.printf("Disconnected from AP: %s, Reason: %d\n", ssid, disconnectionReason);
    Serial.println("Retrying in 2 seconds...");
    delay(2000);
    wifiMulti.start(); // Restart the connection attempt

  });
  wifiMulti.start();
}

void loop() {

}