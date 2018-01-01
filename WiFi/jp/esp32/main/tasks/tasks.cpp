#include "tasks.hpp"
#include "server.hpp"
#include "rom/ets_sys.h"    // ets_delay_us()


void init_server_socket(void)
{
    const uint8_t queue_size  = 5;
    bool server_created       = false;
    uint8_t timeout_count     = 10;
    const uint32_t one_second = 1000 * 1000;

    while (!server_created)
    {
        server_created = tcp_server_init(SERVER_PORT, queue_size);
        if (server_created)
        {
            xEventGroupSetBits( StatusEventGroup,
                                BIT_SERVER_READY );
            ESP_LOGI("init_uart_tx_task", "SUCCESSFULLY initialized server");
            break;
        }
        else
        {
            if (--timeout_count == 0)
            {
                ESP_LOGE("init_uart_tx_task", "FAILED to initialize server");
                break;
            }
            // Retry in 1 second
            ets_delay_us(one_second);
        }
    }
}