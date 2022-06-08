idf_component_register(
    SRCS "network_helpers.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "esp_http_client"
    PRIV_REQUIRES "esp-tls"
)
