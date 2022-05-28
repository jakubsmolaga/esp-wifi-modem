idf_component_register(
    SRCS "wifi_helpers.cpp" "config_server.cpp" "config_page.cpp"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES "esp_http_server"
)
