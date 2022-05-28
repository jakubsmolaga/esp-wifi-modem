#include "wifi_helpers.hpp"

#include "esp_log.h"
#include "string.h"

// Default configuration while becoming an access point
#define SSID "Water Solution"
#define WIFI_CHANNEL 1
#define WIFI_MAX_CONNECTIONS 4

namespace
{
    // Tag used during logging
    static const char *TAG = "CONFIG_SERVER::WIFI_HELPERS";

    // Event handler for WiFi events
    void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        if (event_id == WIFI_EVENT_AP_STACONNECTED)
        {
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
            ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        }
        else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
        {
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
            ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
        }
    }
}

namespace config_server::wifi_helpers
{
    // Initialize WiFi as an access point AND a station
    void wifi_init_apsta()
    {
        // Initialize TCP stack
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_ap();

        // Initialize WiFi (allocate all required resources)
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        // Register event handler for WiFi events
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));

        // Create WiFi configuration
        wifi_ap_config_t ap_config;
        strcpy((char *)ap_config.ssid, SSID);
        ap_config.ssid_len = strlen(SSID);
        ap_config.channel = WIFI_CHANNEL;
        strcpy((char *)ap_config.password, "");
        ap_config.max_connection = WIFI_MAX_CONNECTIONS;
        ap_config.authmode = WIFI_AUTH_OPEN;
        wifi_config_t wifi_config;
        wifi_config.ap = ap_config;

        // Start the WiFi
        ESP_LOGI(TAG, "Initializing wifi:");
        ESP_LOGI(TAG, "\tSSID=%s", SSID);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
        esp_netif_create_default_wifi_sta();
        ESP_LOGI(TAG, "WiFi initialized");
    }

    // Scan for WiFi access points (returns number of APs found and writes results to "result")
    uint16_t wifi_scan(wifi_ap_record_t *result, uint16_t max_result_size)
    {
        // Clear the memmory
        memset(result, 0, max_result_size);

        // Start scanning
        esp_wifi_scan_start(NULL, true);

        // Get results
        uint16_t access_points_found = max_result_size;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&access_points_found, result));
        ESP_LOGI(TAG, "Total APs scanned = %u", access_points_found);

        // Return number of access points found
        return access_points_found;
    }
}
