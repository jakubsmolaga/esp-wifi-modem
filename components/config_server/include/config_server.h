// Callback called when user enters wifi credentials
typedef void (*CallbackFunction)(const char *ssid, const char *password);

// Does the WiFi setup, and starts the server
void config_server_run(CallbackFunction cb);
