#include <esp_http_server.h>
#include <esp_wifi_types.h>

// Sends form with WiFi credentials
void send_config_page(httpd_req_t *req, wifi_ap_record_t *ap_list, uint16_t ap_count);

// Sends information that device is currently trying to connect to a network
void send_connecting_page(httpd_req_t *req);