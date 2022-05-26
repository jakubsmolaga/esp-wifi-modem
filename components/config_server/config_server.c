#include "config_server.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_http_server.h"

#include "wifi_helpers.h"
#include "config_page.h"

#define MAX_SCAN_ACCESS_POINTS 10

static const char *TAG = "CONFIG_SERVER";

static wifi_ap_record_t access_points[MAX_SCAN_ACCESS_POINTS];
static uint8_t access_points_len;

void parse_connection_form(char *body, char *ssid, char *pass)
{
    strtok(body, "&");
    sscanf(body, "ssid=%s", ssid);
    body += strlen(body) + 1;
    sscanf(body, "password=%s", pass);
}

/* -------------------------------------------------------------------------- */
/* --------------------------------- Routes --------------------------------- */
/* -------------------------------------------------------------------------- */

// GET "/" - Returns configuration form
esp_err_t root_get_handler(httpd_req_t *req)
{
    send_config_page(req, access_points, access_points_len);
    return ESP_OK;
}

// POST "/connect" - Calls the callback with WiFi credentials
esp_err_t connect_post_handler(httpd_req_t *req)
{
    // Receive the request body
    const uint8_t max_response_size = 100;
    char response[max_response_size];
    httpd_req_recv(req, response, max_response_size);

    // Send response
    send_connecting_page(req);

    // Parse the request body
    char ssid[50];
    char pass[50];
    parse_connection_form(response, ssid, pass);

    // Call the callback
    CallbackFunction cb = (CallbackFunction)req->user_ctx;
    cb(ssid, pass);

    return ESP_OK;
}

/* -------------------------------------------------------------------------- */
/* ----------------------------- Setup functions ---------------------------- */
/* -------------------------------------------------------------------------- */

// Start the webservers
void start_webserver(CallbackFunction cb)
{
    // Initialize the variables
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    // Register GET "/"
    httpd_uri_t root_handler_config;
    root_handler_config.uri = "/",
    root_handler_config.method = HTTP_GET,
    root_handler_config.handler = root_get_handler,
    httpd_register_uri_handler(server, &root_handler_config);

    // Register POST "/connect"
    httpd_uri_t connect_handler_config;
    connect_handler_config.uri = "/connect";
    connect_handler_config.method = HTTP_POST;
    connect_handler_config.handler = connect_post_handler;
    connect_handler_config.user_ctx = cb;
    httpd_register_uri_handler(server, &connect_handler_config);
}

void config_server_run(CallbackFunction cb)
{
    // Initialize the WiFi
    wifi_init_apsta();

    // Get a list of available access points
    access_points_len = wifi_scan(access_points, MAX_SCAN_ACCESS_POINTS);

    // Start the server
    start_webserver(cb);
}