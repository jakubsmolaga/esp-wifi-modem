#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "string.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_netif.h"

#include "config_server.hpp"
#include "network_helpers.hpp"
#include "storage.hpp"
#include "commands.hpp"

constexpr auto TAG = "MAIN";              // Tag used for logging
constexpr auto max_scanned_networks = 10; // Maximum number of networks found while scanning

auto got_wifi_configuration(const char *ssid, const char *pass) -> void
{
    storage::save_credentials({ssid, pass});
    commands::send_resp("OK");
    esp_restart();
}

/* -------------------------------------------------------------------------- */
/* ---------------------------- Command executors --------------------------- */
/* -------------------------------------------------------------------------- */

auto execute_serve(commands::Command c) -> void
{
    network_helpers::init_wifi_as_apsta("Water Solution");                            // Initialize WiFi as access point + station
    static wifi_ap_record_t networks[max_scanned_networks];                           // Array containging information on networks found
    auto networks_count = network_helpers::scan_wifi(networks, max_scanned_networks); // Scan WiFi for available networks
    config_server::run(got_wifi_configuration, networks, networks_count);             // Run the config server
}

auto execute_connect(commands::Command c) -> void
{
    if (!storage::are_credentails_saved())
        return commands::send_resp("FAIL");
    auto cred = storage::get_credentials();
    if (network_helpers::init_wifi_as_sta(cred.ssid, cred.pass) != ESP_OK)
    {
        commands::send_resp("FAIL");
        storage::forget_credentials();
        return esp_restart();
    }
    commands::send_resp("OK");
}

auto execute_http(commands::Command c) -> void
{
    const auto method = strcmp(c.args[0], "POST") == 0 ? HTTP_METHOD_POST : HTTP_METHOD_GET;
    const auto host = c.args[1];
    const auto path = c.args[2];
    const auto body = (char *)c.data;
    const auto err = network_helpers::make_http_request(method, host, path, body);
    if (err != ESP_OK)
        return commands::send_resp("FAIL");
    return commands::send_resp("OK");
}

/* -------------------------------------------------------------------------- */
/* ---------------------------------- Main ---------------------------------- */
/* -------------------------------------------------------------------------- */

extern "C" void app_main(void)
{
    storage::init();                   // Initialize NVS
    commands::init();                  // Initialize the commands system
    network_helpers::init_tcp_stack(); // Initialize the TCP stack

    // Inform host that the booting process has finished
    commands::send_resp("BOOTED");

    // Run the main loop
    while (true)
    {
        auto c = commands::wait_for_cmd();
        if (strcmp(c.cmd, "SERVE") == 0)
        {
            execute_serve(c);
        }
        else if (strcmp(c.cmd, "CONNECT") == 0)
        {
            execute_connect(c);
        }
        else if (strcmp(c.cmd, "HTTP") == 0)
        {
            execute_http(c);
        }
        else
        {
            commands::send_resp("FAIL");
        }
    }
}
