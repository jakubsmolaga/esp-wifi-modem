#include "esp_wifi.h"

namespace config_server::wifi_helpers
{
    // Initialize WiFi as an access point AND a station
    void wifi_init_apsta();

    // Scan for WiFi access points (returns number of APs found and writes results to "result")
    uint16_t wifi_scan(wifi_ap_record_t *result, uint16_t max_result_size);
}
