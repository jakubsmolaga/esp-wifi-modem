#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

#include "config_server.hpp"
#include "network_helpers.hpp"

static const char *TAG = "MAIN";          // Tag used for logging
constexpr auto max_scanned_networks = 10; // Maximum number of networks found while scanning

// Just prints the credentials entered by the user
void got_wifi_configuration(const char *ssid, const char *password)
{
    ESP_LOGI(TAG, "Got SSID=%s, PASSWORD=%s", ssid, password);
}

/* -------------------------------------------------------------------------- */
/* ---------------------------------- Main ---------------------------------- */
/* -------------------------------------------------------------------------- */

extern "C" void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    network_helpers::init_tcp_stack();                                                // Initialize the TCP stack
    network_helpers::init_wifi_as_apsta("Water Solution");                            // Initialize WiFi as access point + station
    static wifi_ap_record_t networks[10];                                             // Array containging information on networks found
    auto networks_count = network_helpers::scan_wifi(networks, max_scanned_networks); // Scan WiFi for available networks

    // Runs the config server
    config_server::run(got_wifi_configuration, networks, networks_count);
}
