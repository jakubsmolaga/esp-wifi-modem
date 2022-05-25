#include <esp_http_server.h>
#include <esp_wifi_types.h>

void send_config_page(httpd_req_t *req, wifi_ap_record_t *ap_list, uint16_t ap_count);