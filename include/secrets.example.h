#pragma once
// Private values: copy this file to secrets.h (ignored by git) and fill it in.
namespace Secrets {
constexpr char WIFI_SSID[] = "NomeDellaTuaRete";
constexpr char WIFI_PASSWORD[] = "password-del-wifi";     // also the password of the Tonepie-Setup recovery network (min. 8 characters)
constexpr char OTA_PASSWORD[] = "scegli-una-password-lunga";      // required to update the firmware or change Wi-Fi over the network
}
