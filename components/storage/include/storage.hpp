namespace storage
{
    struct WiFiCredentials
    {
        const char *ssid;
        const char *pass;
    };

    auto init() -> void;
    auto are_credentails_saved() -> bool;
    auto get_credentials() -> WiFiCredentials;
    auto save_credentials(WiFiCredentials cred) -> void;
    auto forget_credentials() -> void;
}