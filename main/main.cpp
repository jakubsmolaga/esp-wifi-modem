#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "string.h"

#include "config_server.hpp"
#include "network_helpers.hpp"
#include "storage.hpp"

constexpr auto TAG = "MAIN";              // Tag used for logging
constexpr auto max_scanned_networks = 10; // Maximum number of networks found while scanning

// Just prints the credentials entered by the user
auto got_wifi_configuration(const char *ssid, const char *pass) -> void
{
    ESP_LOGI(TAG, "Got SSID=%s, PASSWORD=%s", ssid, pass);
    storage::save_credentials({ssid, pass});
    esp_restart();
}

auto run_as_station() -> void
{
    auto cred = storage::get_credentials();
    if (network_helpers::init_wifi_as_sta(cred.ssid, cred.pass) != ESP_OK)
    {
        storage::forget_credentials();
        esp_restart();
    }
    ESP_LOGI(TAG, "Connected to a network (%s)", cred.ssid);
}

auto run_as_server() -> void
{
    network_helpers::init_wifi_as_apsta("Water Solution");                            // Initialize WiFi as access point + station
    static wifi_ap_record_t networks[max_scanned_networks];                           // Array containging information on networks found
    auto networks_count = network_helpers::scan_wifi(networks, max_scanned_networks); // Scan WiFi for available networks
    config_server::run(got_wifi_configuration, networks, networks_count);             // Run the config server
}

/* -------------------------------------------------------------------------- */
/* ---------------------------------- Main ---------------------------------- */
/* -------------------------------------------------------------------------- */

extern "C" void app_main(void)
{
    storage::init();                   // Initialize NVS
    network_helpers::init_tcp_stack(); // Initialize the TCP stack
    if (!storage::are_credentails_saved())
        return run_as_server();
    return run_as_station();
}
