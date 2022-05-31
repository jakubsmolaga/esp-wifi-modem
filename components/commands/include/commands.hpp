namespace commands
{
    struct Command
    {
        char *cmd;
        char **args;
        uint8_t args_len;
        uint8_t *data;
        uint16_t data_len;
    };
}
