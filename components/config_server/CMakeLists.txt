idf_component_register(
    SRCS "config_server.cpp" "config_page.cpp"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES "esp_http_server"
)
