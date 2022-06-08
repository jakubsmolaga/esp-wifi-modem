#include "network_helpers.hpp"

#include "string.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_tls.h"
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

    auto _http_event_handler(esp_http_client_event_t *evt) -> esp_err_t
    {
        static char *output_buffer; // Buffer to store response of http request from event handler
        static int output_len;      // Stores number of bytes read
        switch (evt->event_id)
        {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client))
            {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data)
                {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                }
                else
                {
                    if (output_buffer == NULL)
                    {
                        output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL)
                        {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL)
            {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0)
            {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL)
            {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        }
        return ESP_OK;
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

    // Make an http request
    auto make_http_request(esp_http_client_method_t method, const char *host, const char *path, const char *body) -> esp_err_t
    {
        esp_http_client_config_t client_config = {};
        client_config.host = host;
        client_config.path = "/";
        client_config.transport_type = HTTP_TRANSPORT_OVER_TCP;
        client_config.event_handler = _http_event_handler;
        esp_http_client_handle_t client = esp_http_client_init(&client_config);

        esp_http_client_set_url(client, path);
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        if (body != NULL)
        {
            esp_http_client_set_post_field(client, body, strlen(body));
        }
        return esp_http_client_perform(client);
    }
}
