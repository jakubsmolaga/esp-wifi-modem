#include "esp_wifi.h"

namespace config_server
{
    typedef void (*CallbackFunction)(const char *ssid, const char *password);            // Callback called when user enters wifi credentials
    auto run(CallbackFunction cb, wifi_ap_record_t *ap_list, uint16_t ap_count) -> void; // Does the WiFi setup, and starts the server
}
