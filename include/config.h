#pragma once
#include <stdint.h>
#include "secrets.h"
namespace Config {
// Credentials live in secrets.h, which is kept out of the repository.
constexpr auto& WIFI_SSID = Secrets::WIFI_SSID;
constexpr auto& WIFI_PASSWORD = Secrets::WIFI_PASSWORD;
constexpr char HOSTNAME[] = "tonepie";
// Recovery network opened by the module when the home Wi-Fi stays unreachable: http://192.168.4.1
// Same password as the home Wi-Fi above, so it is known even away from this file.
constexpr char AP_SSID[] = "Tonepie-Setup";
constexpr uint32_t AP_AFTER_MS = 180000;
// Required to replace the firmware over the network (tools/ota.py or the /dev page).
constexpr auto& OTA_PASSWORD = Secrets::OTA_PASSWORD;
constexpr char FIRMWARE_VERSION[] = "1.11.0-sync-panel";
constexpr int MCU_RX = 6;
constexpr int MCU_TX = 7;
constexpr uint32_t MCU_BAUD = 115200;
constexpr uint32_t LINK_TIMEOUT_MS = 15000;
constexpr uint32_t DP_FRESH_MS = 60000;
constexpr uint32_t ARM_MS = 600000;
// Network state told to the MCU when Wi-Fi is up: 4 = fully connected (steady LED), 3 = router only (LED blinks).
constexpr uint8_t NETWORK_CONNECTED = 4;
// Visit timestamps. The FRITZ!Box serves NTP locally; the pool is the fallback.
constexpr char TIMEZONE[] = "CET-1CEST,M3.5.0,M10.5.0/3";
constexpr char NTP_PRIMARY[] = "fritz.box";
constexpr char NTP_FALLBACK[] = "pool.ntp.org";
}
