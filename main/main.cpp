#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "string.h"

#include "config_server.hpp"
#include "network_helpers.hpp"

constexpr auto TAG = "MAIN";              // Tag used for logging
constexpr auto max_scanned_networks = 10; // Maximum number of networks found while scanning

// Just prints the credentials entered by the user
auto got_wifi_configuration(const char *ssid, const char *password) -> void
{
    ESP_LOGI(TAG, "Got SSID=%s, PASSWORD=%s", ssid, password);
    nvs_handle_t handle;
    nvs_open("credentials", NVS_READWRITE, &handle);
    nvs_set_str(handle, "password", password);
    nvs_set_str(handle, "ssid", ssid);
    nvs_commit(handle);
    nvs_close(handle);
    esp_restart();
}

auto read_credentials(char *ssid, char *pass) -> void
{
    nvs_handle_t handle;
    nvs_open("credentials", NVS_READONLY, &handle);
    size_t pass_len;
    nvs_get_str(handle, "password", pass, &pass_len);
    size_t ssid_len;
    nvs_get_str(handle, "ssid", ssid, &ssid_len);
    nvs_close(handle);
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

    char ssid[50] = "";
    char pass[50] = "";
    read_credentials(ssid, pass);
    ESP_LOGI(TAG, "ssid=%s, pass=%s", ssid, pass);

    network_helpers::init_tcp_stack(); // Initialize the TCP stack
    if (strlen(ssid) != 0 && strlen(pass) != 0)
    {
        network_helpers::init_wifi_as_sta(ssid, pass);
    }
    else
    {
        network_helpers::init_wifi_as_apsta("Water Solution");                            // Initialize WiFi as access point + station
        static wifi_ap_record_t networks[10];                                             // Array containging information on networks found
        auto networks_count = network_helpers::scan_wifi(networks, max_scanned_networks); // Scan WiFi for available networks

        // Runs the config server
        config_server::run(got_wifi_configuration, networks, networks_count);
    }
}
