#include "esp_wifi.h"
#include "esp_http_client.h"

namespace network_helpers
{
    auto init_tcp_stack() -> void;                                                  // Initialize the TCP stack (Call this before any other networking)
    auto init_wifi_as_apsta(const char *ap_ssid) -> void;                           // Start WiFi as access point + station
    auto init_wifi_as_sta(const char *ssid, const char *pass) -> esp_err_t;         // Start WiFi as a station
    auto scan_wifi(wifi_ap_record_t *result, uint16_t max_result_size) -> uint16_t; // Scan for WiFi networks
    auto make_http_request(esp_http_client_method_t method, const char *host, const char *path, const char *body) -> esp_err_t;
}