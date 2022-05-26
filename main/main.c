#include "esp_log.h"
#include "nvs_flash.h"

#include "config_server.h"

// Tag used for logging
static const char *TAG = "MAIN";

// Just prints the credentials entered by the user
void got_wifi_configuration(const char *ssid, const char *password)
{
    ESP_LOGI(TAG, "Got SSID=%s, PASSWORD=%s", ssid, password);
}

/* -------------------------------------------------------------------------- */
/* ---------------------------------- Main ---------------------------------- */
/* -------------------------------------------------------------------------- */

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Runs the config server
    config_server_run(got_wifi_configuration);
}
