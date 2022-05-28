namespace config_server
{
    // Callback called when user enters wifi credentials
    typedef void (*CallbackFunction)(const char *ssid, const char *password);

    // Does the WiFi setup, and starts the server
    void run(CallbackFunction cb);
}
