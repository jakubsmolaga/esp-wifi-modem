#include "inttypes.h"

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

    auto init() -> void;
    auto send_resp(const char *response) -> void;
    auto wait_for_cmd() -> Command;
    auto read_something(char *buf, uint16_t max_len) -> void;
}
