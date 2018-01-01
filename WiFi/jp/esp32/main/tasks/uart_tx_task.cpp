#include "tasks.hpp"
// Framework libraries
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "driver/gpio.h"
// Project libraries
#include "packet_parser.hpp"


static void transfer_command_packet(command_packet_S *packet)
{
    uint8_t *buffer = (uint8_t *)&packet;
    uint8_t bytes   = uart_write_bytes(UART_NUMBER, (const char *)buffer, sizeof(command_packet_S));
    if (bytes != sizeof(command_packet_S))
    {
        ESP_LOGE("transfer_command_packet", "Only sent %d / %d bytes", bytes, sizeof(command_packet_S));
    }
#if TESTING
    else
    {
        ESP_LOGI("transfer_command_packet", "Type: %x | Opcode: %x | Command: %x", 
                        packet->type, packet->opcode, packet->command.half_word);
    }
#endif
}

void uart_tx_task(void *p)
{
    // Command packet
    command_packet_S packet = { };

    // Wait for server to be created before starting
    xEventGroupWaitBits(StatusEventGroup,   // Event group handle
                        BIT_SERVER_READY,   // Bits to wait for
                        false,              // Clear on exit
                        pdTRUE,             // Wait for all bits
                        TICK_MS(ONE_MIN));  // Ticks to wait

    // Main loop
    while (1)
    {
        // Block until receive packet from accept_task
        xQueueReceive(ServerQueue, &packet, MAX_DELAY);

        // Send over UART
        transfer_command_packet(&packet);
    }
}