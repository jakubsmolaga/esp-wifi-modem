idf_component_register(
    SRCS "wifi_helpers.c" "config_server.c" "config_page.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES "esp_http_server"
)
