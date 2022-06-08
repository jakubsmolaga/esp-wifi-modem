#include "commands.hpp"

#include "driver/uart.h"
#include "esp_err.h"
#include "string.h"
#include "esp_vfs_dev.h"

constexpr auto UART_PORT_NUM = 0;
constexpr auto UART_RX_PIN = 3;
constexpr auto UART_TX_PIN = 1;
constexpr auto BUF_SIZE = 1024;

namespace
{
    auto str_has_prefix(const char *s, const char *prefix) -> bool
    {
        if (strlen(prefix) > strlen(s))
            return false;
        return memcmp(s, prefix, strlen(prefix)) == 0;
    }

    auto strip_cr_lf(char *s) -> void
    {
        const auto len = strlen(s);
        if (s[len - 2] == '\r')
            s[len - 2] = '\0';
        if (s[len - 1] == '\n')
            s[len - 1] = '\0';
    }
}

namespace commands
{
    auto init() -> void
    {
        auto config = uart_config_t{};
        config.baud_rate = 115200;
        config.data_bits = UART_DATA_8_BITS;
        config.parity = UART_PARITY_DISABLE;
        config.stop_bits = UART_STOP_BITS_1;
        config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        config.source_clk = UART_SCLK_APB;
        ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
        ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &config));
        ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        esp_vfs_dev_uart_use_driver(UART_PORT_NUM);
        esp_vfs_dev_uart_port_set_rx_line_endings(UART_PORT_NUM, ESP_LINE_ENDINGS_CRLF);
        esp_vfs_dev_uart_port_set_tx_line_endings(UART_PORT_NUM, ESP_LINE_ENDINGS_CRLF);
    }

    auto send_resp(const char *response) -> void
    {
        printf("ESP_RESP %s\n", response);
    }

    auto wait_for_cmd() -> Command
    {
        // Create the structure
        static char *args_buf[10];
        Command c = {
            .cmd = NULL,
            .args = args_buf,
            .args_len = 0,
            .data = NULL,
            .data_len = 0,
        };

        // Find a line starting with "ESP_CMD"
        constexpr auto max_line_len = 200;       // Maximum line length
        static char line[max_line_len];          // Buffer holding the line
        strcpy(line, "");                        // Initialize the buffer
        while (!str_has_prefix(line, "ESP_CMD")) // Repeat while buffer doesn't start with "ESP_CMD"
            fgets(line, max_line_len, stdin);    // Read a line
        strip_cr_lf(line);                       // Strip end of line symbols

        // Parse the command
        char *buf = line;          // Pointer to the currently parsed token
        strsep(&buf, " ");         // Consume "ESP_CMD" prefix
        c.cmd = strsep(&buf, " "); // Get the command

        // Parse arguments
        while (buf != NULL)
        {
            c.args[c.args_len] = strsep(&buf, " ");
            c.args_len += 1;
        }

        // Return if there is no data to read
        if (strcmp(c.args[c.args_len - 1], "ESP_DATA_BEGIN") != 0)
            return c;

        // Remove "ESP_DATA_BEGIN" from the argument list
        c.args_len -= 1;

        // Create the data buffer
        constexpr auto max_data_len = 500;
        static char data_line[max_line_len] = "";
        static uint8_t data[max_data_len] = "";
        c.data = data;

        // Read lines until "ESP_DATA_END"
        while (strcmp(data_line, "ESP_DATA_END") != 0)
        {
            fgets(data_line, max_line_len, stdin);
            strcat((char *)data, data_line);
            strip_cr_lf(data_line);
            c.data_len = strlen(data_line);
        }

        // Remove "ESP_DATA_END" from the data buffer
        strip_cr_lf((char *)data);
        data[strlen((char *)data) - strlen("ESP_DATA_END")] = '\0';
        c.data_len = strlen((char *)data);

        // Return the command
        return c;
    }
}