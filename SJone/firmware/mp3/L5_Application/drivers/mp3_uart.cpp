#include "mp3_uart.hpp"
// Standard libraries
#include <stdio.h>
// Framework libraries
#include "sys_config.h" // sys_get_cpu_clock() = 48MHz default


// Baud rate
#define DEFAULT_BAUDRATE (115200)

// Interrupt Enable Bits
#define IER_RBR_BIT     (1 << 0)    // RBR interrupt enable
#define IER_THRE_BIT    (1 << 1)    // THRE interrupt enable
#define IER_RX_LSR_BIT  (1 << 2)    // RX line status interrupt enable
// Interrupt Status Bits
#define IIR_RX_LSR_BIT  (0x3 << 1)  // RX line status error
#define IIR_RXREADY_BIT (1   << 2)  // RX data available
#define IIR_TIO_BIT     (0x3 << 2)  // Character time out indication
#define IIR_THRE_BIT    (1   << 1)  // THRE
// LCR Divisor Latch Access Bit
#define LCR_DLAB_BIT    (1 << 7)    // Disable before configuration
// Line Status Register bits
#define LSR_RDR_BIT     (1 << 0)    // Set when RX FIFO is not empty
#define LSR_OE_BIT      (1 << 1)    // Overrun flag, means new data is lost
#define LSR_PE_BIT      (1 << 2)    // Parity error
#define LSR_FE_BIT      (1 << 3)    // Framing error, incorrect stop bit, unsynchronized
#define LSR_BI_BIT      (1 << 4)    // Break interrupt
#define LSR_THRE_BIT    (1 << 5)    // Transmit holding register empty
#define LSR_TEMT_BIT    (1 << 6)    // Both THR and TSR (Transmit Shift Register) are empty
#define LSR_RXFE_BIT    (1 << 7)    // RBR contains an error (framing, parity, break interrupt)

// Extern
QueueHandle_t UartRxQueue;
QueueHandle_t UartTxQueue;

// Interrupt Handlers
extern "C" 
{
    void uart_interupt_callback(LPC_UART_TypeDef *uart_ptr)
    {
        // Read register without using
        asm volatile ("" : : "r" (uart_ptr->LSR));

        long higher_priority_task_woken[2] = { 0, 0 };
        int interrupt_type = uart_ptr->IIR & 0xF;
        uint8_t byte = 0;

        switch (interrupt_type)
        {
            // RX Data Available
            case IIR_RXREADY_BIT: // No break
            // Character Time Out Indication
            case IIR_TIO_BIT:
                while (uart_ptr->LSR & LSR_RDR_BIT)
                {
                    byte = uart_ptr->RBR;
                    xQueueSendFromISR(UartRxQueue, &byte, &higher_priority_task_woken[0]);
                }
                break;

            // Transmit Holding Register Empty
            case IIR_THRE_BIT:
                // Since hardware FIFO is 16 bytes, service all available bytes in FIFO before exiting interrupt
                for (int i=0; i<16; i++)
                {
                    // Exit if no more bytes in the queue
                    if (!xQueueReceiveFromISR(UartTxQueue, &byte, &higher_priority_task_woken[0])) 
                    {
                        break;
                    }
                    uart_ptr->THR = byte;
                }
                break;

            // RX Line Status Error
            case IIR_RX_LSR_BIT: // No break
            // Unhandled
            default: 
                break;
        }

        portYIELD_FROM_ISR( higher_priority_task_woken );
    }

    void UART3_IRQHandler()
    {
        uart_interupt_callback(LPC_UART3);
    }
}

void Init_Uart(void)
{
    // Create Uart queues
    UartRxQueue = xQueueCreate(sizeof(command_packet_S)    * 4, sizeof(char)); // 4 command packets in bytes
    UartTxQueue = xQueueCreate(sizeof(diagnostic_packet_S) * 2, sizeof(char)); // 2 diagnostic packets in bytes

    LPC_SC->PCONP        |=   (1   << 25);
    LPC_SC->PCLKSEL1     &=  ~(3   << 18);
    LPC_SC->PCLKSEL1     |=   (1   << 18);
    LPC_PINCON->PINSEL9  &=  ~(0xF << 24); // Clear values
    LPC_PINCON->PINSEL9  |=   (0xF << 24); // Set values for UART3 Rx/Tx

    // Same configuration across all UART, avoid repetition
    // Clear and enable FIFOs
    LPC_UART3->FCR = ( (1 << 2) | (1 << 1) | (1 << 0) );

    // Enable DLAB before configuration
    LPC_UART3->LCR = LCR_DLAB_BIT;
    { 
        // Set baud rate divisors
        uint16_t baud  = (sys_get_cpu_clock() / (16 * DEFAULT_BAUDRATE)) + 0.5;
        LPC_UART3->DLM = baud >> 8;
        LPC_UART3->DLL = baud & 0xFF;
    }
    // Disable DLAB
    LPC_UART3->LCR &= ~(LCR_DLAB_BIT);

    // 8-bit character, 1 stop bit, no parity, no break, disable DLAB
    LPC_UART3->LCR = 0x3;

    // Enable interrupts for RBR, THRE, and RX LSR
    NVIC_EnableIRQ(UART3_IRQn);
    LPC_UART3->IER = ( IER_RBR_BIT | IER_THRE_BIT | IER_RX_LSR_BIT );

    LOG_STATUS("Uart initialized.\n");
}

// bool uart_rx_byte(uint8_t &byte, uint32_t timeout_ms)
// {
//     return xQueueReceive(UartRxQueue, &byte, TICK_MS(timeout_ms));
// }

bool uart_tx_byte(uint8_t byte, TickType_t timeout_ms)
{
    return xQueueReceive(UartRxQueue, &byte, timeout_ms);
}