#include "network_helpers.hpp"

#include "string.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "lwip/sys.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

namespace
{
    using namespace network_helpers;

    constexpr auto TAG = "NETWORK_HELPERS";
    static EventGroupHandle_t s_wifi_event_group;
    static int s_retry_num = 0;

    // Event handler for WiFi events
    auto wifi_ap_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) -> void
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

    auto wifi_sta_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) -> void
    {
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
        {
            esp_wifi_connect();
        }
        else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            if (s_retry_num < 5)
            {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "retry to connect to the AP");
            }
            else
            {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
            ESP_LOGI(TAG, "connect to the AP fail");
        }
        else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
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
                                                            &wifi_ap_event_handler,
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

    // Initialize WiFi as a station
    auto init_wifi_as_sta(const char *ssid, const char *pass) -> esp_err_t
    {
        s_wifi_event_group = xEventGroupCreate();
        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_sta_event_handler,
                                                            NULL,
                                                            &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifi_sta_event_handler,
                                                            NULL,
                                                            &instance_got_ip));

        wifi_config_t wifi_config = {};
        strcpy((char *)wifi_config.sta.ssid, ssid);
        strcpy((char *)wifi_config.sta.password, pass);
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "wifi_init_sta finished.");

        /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
         * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               portMAX_DELAY);

        /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
         * happened. */
        if (!(bits & WIFI_CONNECTED_BIT))
            return ESP_FAIL;
        return ESP_OK;
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
