/*
* You need to copy this file to your project `test` folder as `aps_configuration.h`,
* And fill the right SSID/Passphrase values for your test environment.
* You'll need two APs, one strong and one weak.
* The goal is to test that `AsncWiFiMulti` connects to the strongest AP when provided with the correct credentials.
* If the strongest AP has an invalid passphrase, it should connect to the weaker one with a valid passphrase.
*/
#pragma once

#define STRONG_VALID_AP_SSID "My_SSID"
#define STRONG_VALID_AP_PASSPHRASE "MyPassphrase"

#define WEAK_VALID_AP_SSID "My_Weak_SSID"
#define WEAK_VALID_AP_PASSPHRASE "MyPassphrase"
