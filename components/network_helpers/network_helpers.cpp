#include "network_helpers.hpp"

#include "string.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"

namespace
{
    constexpr auto TAG = "NETWORK_HELPERS";

    // Event handler for WiFi events
    auto wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) -> void
    {
        if (event_id == WIFI_EVENT_AP_STACONNECTED)
        {
            auto event = (wifi_event_ap_staconnected_t *)event_data;
            ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        }
        else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
        {
            auto event = (wifi_event_ap_stadisconnected_t *)event_data;
            ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
        }
    }
}

namespace network_helpers
{
    // Initialize TCP stack. Run this function before any other networking begins
    auto init_tcp_stack() -> void
    {
        ESP_ERROR_CHECK(esp_netif_init());                // Initialize the networking interface
        ESP_ERROR_CHECK(esp_event_loop_create_default()); // Create default event loop
    }

    // Initialize WiFi as access point + station
    auto init_wifi_as_apsta(const char *ap_ssid) -> void
    {
        // Initialize WiFi (allocate all required resources)
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        // Create default interfaces
        esp_netif_create_default_wifi_ap();
        esp_netif_create_default_wifi_sta();

        // Register event handler for WiFi events
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));

        // Create WiFi configuration
        wifi_ap_config_t ap_config;
        strcpy((char *)ap_config.ssid, ap_ssid);
        ap_config.ssid_len = strlen(ap_ssid);
        ap_config.channel = 1;
        strcpy((char *)ap_config.password, "");
        ap_config.max_connection = 4;
        ap_config.authmode = WIFI_AUTH_OPEN;
        wifi_config_t wifi_config;
        wifi_config.ap = ap_config;

        // Start the WiFi
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
    }

    // Scan for WiFi networks
    auto scan_wifi(wifi_ap_record_t *result, uint16_t max_result_size) -> uint16_t
    {
        memset(result, 0, max_result_size); // Clear the memmory
        esp_wifi_scan_start(NULL, true);    // Start scanning

        // Get results
        uint16_t access_points_found = max_result_size;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&access_points_found, result));

        return access_points_found;
    }
}
