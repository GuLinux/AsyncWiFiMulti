#include <AsyncWiFiMulti.h>
#include <Arduino.h>
#include <unity.h>
#include "aps_configuration.h"

using namespace GuLinux;

void await_until(const char *error_message, std::function<bool()> condition, uint16_t max_seconds = 60, uint16_t delay_ms = 100, const char *interval_message = nullptr) {
  uint16_t attempts = 0;
  auto started_at = millis();
  while(!condition()) {
    if(millis() - started_at > max_seconds * 1000) {
      TEST_FAIL_MESSAGE(error_message);
      return;
    }
    if(interval_message) {
      Serial.printf("[%0.2f/%d] %s\n", (millis()-started_at) / 1000.0f, max_seconds, interval_message);
    }
    delay(delay_ms);
  }
}


void test_should_complete_scan() {
  AsyncWiFiMulti wifiMulti;
  wifiMulti.addAP(WEAK_VALID_AP_SSID, "InvalidPassphrase#1");
  wifiMulti.addAP(STRONG_VALID_AP_SSID, "InvalidPassphrase#2");
  wifiMulti.addAP("AnyInvalidSSID", "InvalidPassphrase#3");

  wifiMulti.start();
  uint16_t attempts = 0;
  await_until("Scan did not complete in time", [&wifiMulti]() {
    return !wifiMulti.getFoundAPs().empty();
  }, 60, 500, "Waiting for scan to complete...");

  TEST_ASSERT_EQUAL_UINT64(2, wifiMulti.getFoundAPs().size());
  auto it = wifiMulti.getFoundAPs().begin();

  TEST_ASSERT_EQUAL_STRING(it->ssid.c_str(), STRONG_VALID_AP_SSID);
  TEST_ASSERT_EQUAL_STRING(it++->passphrase.c_str(), "InvalidPassphrase#2");
  TEST_ASSERT_EQUAL_STRING(it->ssid.c_str(), WEAK_VALID_AP_SSID);
  TEST_ASSERT_EQUAL_STRING(it->passphrase.c_str(), "InvalidPassphrase#1");
}


void test_should_connect_to_strongest_wifi() {
  AsyncWiFiMulti wifiMulti;
  wifiMulti.addAP(WEAK_VALID_AP_SSID, WEAK_VALID_AP_PASSPHRASE);
  wifiMulti.addAP(STRONG_VALID_AP_SSID, STRONG_VALID_AP_PASSPHRASE);

  bool onConnectedCalled = false;
  bool onFailureCalled = false;

  wifiMulti.onConnected([&onConnectedCalled](const AsyncWiFiMulti::ApSettings &ap) {
    onConnectedCalled = true;
    TEST_ASSERT_EQUAL_STRING(STRONG_VALID_AP_SSID, ap.ssid.c_str());
    TEST_ASSERT_EQUAL_STRING(STRONG_VALID_AP_PASSPHRASE, ap.passphrase.c_str());
  });
  wifiMulti.onFailure([&onFailureCalled]() {
    onFailureCalled = true;
  });


  wifiMulti.start();
  await_until("WiFi connection did not complete in time", []() {
    return WiFi.status() == WL_CONNECTED;
  }, 60, 1000, "Waiting for WiFi connection...");
  TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
  TEST_ASSERT_EQUAL_STRING(STRONG_VALID_AP_SSID, WiFi.SSID().c_str());
  TEST_ASSERT_EQUAL_STRING(STRONG_VALID_AP_PASSPHRASE, WiFi.psk().c_str());

  await_until("onConnected callback was not called", [&onConnectedCalled]() {
    return onConnectedCalled;
  }, 60, 1000, "Waiting for onConnected callback...");

  TEST_ASSERT_FALSE(onFailureCalled);
}


void test_should_weaker_wifi_when_strongest_password_is_invalid() {
  AsyncWiFiMulti wifiMulti;
  wifiMulti.addAP(STRONG_VALID_AP_SSID, "InvalidPassphrase");
  wifiMulti.addAP(WEAK_VALID_AP_SSID, WEAK_VALID_AP_PASSPHRASE);

  bool onConnectedCalled = false;
  bool onFailureCalled = false;

  wifiMulti.onConnected([&onConnectedCalled](const AsyncWiFiMulti::ApSettings &ap) {
    onConnectedCalled = true;
    TEST_ASSERT_EQUAL_STRING(WEAK_VALID_AP_SSID, ap.ssid.c_str());
    TEST_ASSERT_EQUAL_STRING(WEAK_VALID_AP_PASSPHRASE, ap.passphrase.c_str());
  });
  wifiMulti.onFailure([&onFailureCalled]() {
    onFailureCalled = true;
  });


  wifiMulti.start();
  await_until("WiFi connection did not complete in time", []() {
    return WiFi.status() == WL_CONNECTED;
  }, 90, 1000, "Waiting for WiFi connection...");
  TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
  TEST_ASSERT_EQUAL_STRING(WEAK_VALID_AP_SSID, WiFi.SSID().c_str());
  TEST_ASSERT_EQUAL_STRING(WEAK_VALID_AP_PASSPHRASE, WiFi.psk().c_str());

  await_until("onConnected callback was not called", [&onConnectedCalled]() {
    return onConnectedCalled;
  }, 60, 1000, "Waiting for onConnected callback...");
  TEST_ASSERT_FALSE(onFailureCalled);
}


void test_should_call_failure_callback_when_not_connecting() {
  AsyncWiFiMulti wifiMulti;
  wifiMulti.addAP(STRONG_VALID_AP_SSID, "InvalidPassphrase");
  wifiMulti.addAP(WEAK_VALID_AP_SSID, "InvalidPassphrase");

  bool onConnectedCalled = false;
  bool onFailureCalled = false;

  wifiMulti.onConnected([&onConnectedCalled](const AsyncWiFiMulti::ApSettings &ap) {
    onConnectedCalled = true;
  });
  wifiMulti.onFailure([&onFailureCalled]() {
    onFailureCalled = true;
  });


  wifiMulti.start();

  await_until("onFailure callback was not called", [&onFailureCalled]() {
    return onFailureCalled;
  }, 60, 1000, "Waiting for onConnected callback...");
  TEST_ASSERT_FALSE(onConnectedCalled); 
}


void test_should_run_on_disconnected_callback_if_disconnected_after_finished() {
  AsyncWiFiMulti wifiMulti;
  wifiMulti.addAP(STRONG_VALID_AP_SSID, STRONG_VALID_AP_PASSPHRASE);

  bool onConnectedCalled = false;
  bool onDisconnectedCalled = false;

  wifiMulti.onConnected([&onConnectedCalled](const AsyncWiFiMulti::ApSettings &ap) {
    onConnectedCalled = true;
  });
  wifiMulti.onDisconnected([&onDisconnectedCalled](const char *ssid, uint8_t disconnectionReason) {
    TEST_ASSERT_EQUAL_STRING(STRONG_VALID_AP_SSID, ssid);
    TEST_ASSERT_EQUAL(WIFI_REASON_ASSOC_LEAVE, disconnectionReason);
    onDisconnectedCalled = true;
  });

  wifiMulti.start();
  await_until("WiFi connection did not complete in time", []() {
    return WiFi.status() == WL_CONNECTED;
  }, 60, 1000, "Waiting for WiFi connection...");

  TEST_ASSERT_TRUE(onConnectedCalled);

  delay(2000);

  WiFi.disconnect(false);

  await_until("onDisconnected callback was not called", [&onDisconnectedCalled]() {
    return onDisconnectedCalled;
  }, 10, 1000, "Waiting for onDisconnected callback...");
  
  TEST_ASSERT_TRUE(onDisconnectedCalled);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Starting AsyncWiFiMulti tests");
  UNITY_BEGIN();
  RUN_TEST(test_should_complete_scan);
  RUN_TEST(test_should_connect_to_strongest_wifi);
  RUN_TEST(test_should_weaker_wifi_when_strongest_password_is_invalid);
  RUN_TEST(test_should_call_failure_callback_when_not_connecting);
  RUN_TEST(test_should_run_on_disconnected_callback_if_disconnected_after_finished);
  UNITY_END(); 
}

void loop() {
  delay(1000);
}