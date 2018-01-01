#include "uart.hpp"
// Framework libraries
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "driver/gpio.h"
// Project libraries
#include "common.hpp"


void init_uart(void)
{
    // Configure UART
    uart_config_t uart_config = { };
    uart_config.baud_rate = BAUD_RATE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity    = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

    uart_param_config(UART_NUMBER, &uart_config);
    uart_set_pin(UART_NUMBER, TX_GPIO_NUM, RX_GPIO_NUM, 0, 0);
    uart_driver_install(UART_NUMBER, MAX_BUFFER_SIZE, 0, 0, NULL, 0);
}

#if 0
// send command bytes to SJSUOne
void socket_to_uart_task(void *p)
{
    const uint8_t PLAY[] = { PACKET_TYPE_COMMAND_WRITE, PACKET_OPCODE_SET_PLAY_CURRENT, 0x56, 0x56 };
    const uint8_t NEXT[] = { PACKET_TYPE_COMMAND_WRITE, PACKET_OPCODE_SET_PLAY_NEXT,    0x56, 0x56 };
    const uint8_t PREV[] = { PACKET_TYPE_COMMAND_WRITE, PACKET_OPCODE_SET_PLAY_PREV,    0x56, 0x56 };

    while(1)
    {
        uint8_t bytes = uart_write_bytes(UART_NUMBER, (const char *)PLAY , strlen((const char*)PLAY));
        ESP_LOGI("uart", "Sent: %s\n", PLAY);
        ESP_LOGI("bytes", "%d\n", bytes);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        bytes = uart_write_bytes(UART_NUMBER, (const char *)NEXT , strlen((const char*)NEXT));
        ESP_LOGI("uart", "Sent: %s\n", NEXT);
        ESP_LOGI("bytes", "%d\n", bytes);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// send command bytes to SJSUOne
void uart_to_socket_task(void *p)
{
    uint8_t buffer[256] = { 0 };
    while (1)
    {
        int ret = uart_read_bytes(UART_NUMBER, buffer, 256, portMAX_DELAY);
        if (ret != -1)
        {
            printf("%s\n", buffer);
        }
        memset(buffer, 0, 256);
    }
}
#endif