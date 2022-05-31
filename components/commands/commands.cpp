#include "commands.hpp"

#include "driver/uart.h"
#include "esp_err.h"
#include "string.h"

constexpr auto UART_PORT_NUM = 0;
constexpr auto UART_RX_PIN = 20;
constexpr auto UART_TX_PIN = 21;
constexpr auto BUF_SIZE = 1024;

namespace
{
    template <typename... Args>
    auto send_fmt(const char *fmt, Args... args) -> void
    {
        static char buf[200];
        auto len = sprintf(buf, fmt, args...);
        uart_write_bytes(UART_PORT_NUM, buf, len);
    }

    auto read_line(char *buf, uint16_t max_len) -> int
    {
        auto i = 0;
        do
        {
            i += uart_read_bytes(UART_PORT_NUM, buf + i, max_len - 1, portTICK_PERIOD_MS * 10000);
        } while (strstr(buf, "\r\n") != NULL);
        return i;
    }

    auto str_has_prefix(const char *s, const char *prefix) -> bool
    {
        if (strlen(prefix) > strlen(s))
            return false;
        return memcmp(s, prefix, strlen(prefix) - 1);
    }
}

namespace commands
{
    auto init() -> void
    {
        auto config = uart_config_t{
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_APB,
        };
        ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
        ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &config));
        ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    }

    auto send_resp(const char *response) -> void
    {
        send_fmt("ESP_RESP %s\r\n", response);
    }

    auto wait_for_cmd() -> Command
    {
        constexpr auto buf_len = 200;
        static char buf[buf_len];
        read_line(buf, buf_len);
        if (!str_has_prefix(buf, "ESP_CMD"))
            return wait_for_cmd();

        strtok(buf, " ");
        static char *args_buf[10];
        Command c = {
            .cmd = strtok(NULL, " "),
            .args = args_buf,
            .args_len = 0,
            .data = NULL,
            .data_len = 0,
        };

        c.args[c.args_len] = strtok(NULL, " ");
        while (c.args[c.args_len] != NULL)
        {
            c.args_len += 1;
            c.args[c.args_len] = strtok(NULL, " ");
        }

        if (strcmp(c.args[c.args_len - 1], "ESP_DATA_BEGIN") != 0)
            return c;

        constexpr auto max_data_len = 500;
        static uint8_t data_buf[max_data_len];
        c.args_len -= 1;
        c.data = data_buf;
        while (true)
        {
            auto len = read_line((char *)c.data, max_data_len);
            if (strcmp((char *)(c.data + c.data_len), "ESP_DATA_END") == 0)
                break;
            c.data_len += len;
        }

        return c;
    }
}