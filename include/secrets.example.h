#pragma once
// Private values: copy this file to secrets.h (ignored by git) and fill it in.
namespace Secrets {
constexpr char WIFI_SSID[] = "YourWifiName";
constexpr char WIFI_PASSWORD[] = "your-wifi-password";     // also the password of the Tonepie-Setup recovery network (min. 8 characters)
constexpr char OTA_PASSWORD[] = "choose-a-long-update-password";      // required to update the firmware or change Wi-Fi over the network
}
