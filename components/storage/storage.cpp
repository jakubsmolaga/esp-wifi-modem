#include "storage.hpp"

#include "nvs_flash.h"
#include "string.h"

namespace
{
    constexpr auto CREDENTIALS_NAMESPACE = "credentials";
}

namespace storage
{
    auto init() -> void
    {
        auto err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);
    }

    auto are_credentails_saved() -> bool
    {
        auto cred = get_credentials();
        return strlen(cred.ssid) > 0;
    }

    // Get saved credentials
    auto get_credentials() -> WiFiCredentials
    {
        static char ssid[50] = "";
        static char pass[50] = "";

        // Open NVS
        nvs_handle_t handle;
        nvs_open(CREDENTIALS_NAMESPACE, NVS_READONLY, &handle);

        // Read SSID
        size_t ssid_len = 50;
        nvs_get_str(handle, "ssid", ssid, &ssid_len);

        // Read password
        size_t pass_len = 50;
        nvs_get_str(handle, "pass", pass, &pass_len);

        // Close NVS
        nvs_close(handle);

        // Return credentials
        return WiFiCredentials{
            .ssid = ssid,
            .pass = pass,
        };
    }

    auto save_credentials(WiFiCredentials cred) -> void
    {
        nvs_handle_t handle;
        nvs_open(CREDENTIALS_NAMESPACE, NVS_READWRITE, &handle);
        nvs_set_str(handle, "ssid", cred.ssid);
        nvs_set_str(handle, "pass", cred.pass);
        nvs_commit(handle);
        nvs_close(handle);
    }

    auto forget_credentials() -> void
    {
        nvs_handle_t handle;
        nvs_open(CREDENTIALS_NAMESPACE, NVS_READWRITE, &handle);
        nvs_erase_all(handle);
        nvs_commit(handle);
        nvs_close(handle);
    }
}