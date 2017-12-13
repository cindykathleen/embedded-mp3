#include "uart.hpp"
#include <stdio.h>
#include <string>
#include <cassert>
#include "sys_config.h"
#include "task.h"
#include "queue.h"
#include "L0_LowLevel/source/lpc_peripherals.h"

static QueueHandle_t RxQueue = xQueueCreate(130, sizeof(char));
static QueueHandle_t TxQueue = xQueueCreate(130, sizeof(char));

// Interrupt Handlers
extern "C" 
{
    void UART2_IRQHandler()
    {
        // Read register without using
        asm volatile ("" : : "r" (LPC_UART2->LSR));

        long higher_priority_task_woken = 0;
        int interrupt_type = LPC_UART2->IIR & 0x0000000F;

        switch (interrupt_type)
        {
            // RX Line Status Error
            case IIR_RX_LSR_BIT:
                break;

            // RX Data Available
            case IIR_RXREADY_BIT: 
                // No break
            
            // Character Time Out Indication
            case IIR_TIO_BIT:
                while (LPC_UART2->LSR & (1 << 0)) {
                    uint8_t byte = LPC_UART2->RBR;
                    xQueueSendFromISR(RxQueue, &byte, &higher_priority_task_woken);
                }
                break;

            // THRE
            case IIR_THRE_BIT:
                uint8_t byte;
                for (int i=0; i<16; i++) {
                    if (!xQueueReceiveFromISR(TxQueue, &byte, &higher_priority_task_woken)) {
                        break;
                    }
                    LPC_UART2->THR = byte;
                }
                break;

            // Unhandled
            default: break;
        }

        portYIELD_FROM_ISR( higher_priority_task_woken );
    }

    void UART3_IRQHandler()
    {
        // Read register without using
        asm volatile ("" : : "r" (LPC_UART3->LSR));
        
        long higher_priority_task_woken = 0;
        int interrupt_type = LPC_UART3->IIR & 0x0000000F;

        switch (interrupt_type)
        {
            // RX Line Status Error
            case IIR_RX_LSR_BIT:
                break;

            // RX Data Available
            case IIR_RXREADY_BIT: 
                // No break

            // Character Time Out Indication
            case IIR_TIO_BIT:
                while (LPC_UART3->LSR & (1 << 0)) 
                {
                    uint8_t byte = LPC_UART3->RBR;
                    xQueueSendFromISR(RxQueue, &byte, &higher_priority_task_woken);
                }
                break;

            // THRE
            case IIR_THRE_BIT:
                uint8_t byte;
                for (int i=0; i<16; i++) 
                {
                    if (!xQueueReceiveFromISR(TxQueue, &byte, &higher_priority_task_woken)) 
                    {
                        break;
                    }
                    LPC_UART3->THR = byte;
                }
                break;

            // Unhandled
            default: break;
        }

        portYIELD_FROM_ISR( higher_priority_task_woken );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Uart::Uart(uart_port_t port)
{
    Port = port;

    switch (Port)
    {
        // case UART_PORT0: 
        //     UartPtr = (LPC_UART_TypeDef *)LPC_UART0; 
        //     IRQPtr  = UART0_IRQn;
        //     break;
        case UART_PORT2: 
            UartPtr = LPC_UART2; 
            IRQPtr  = UART2_IRQn;
            break;
        case UART_PORT3: 
            UartPtr = LPC_UART3; 
            IRQPtr  = UART3_IRQn;
            break;
    }
}

void Uart::Init(uint32_t baud_rate)
{
    switch (Port)
    {
        // case UART_PORT0:
        //     LPC_SC->PCONP       |=  (1 << 3);
        //     LPC_SC->PCLKSEL0    &= ~(3 << 6);
        //     LPC_SC->PCLKSEL0    |=  (1 << 6);
        //     LPC_PINCON->PINSEL0 &= ~(0xF << 4);
        //     LPC_PINCON->PINSEL0 |=  (0x5 << 4);
        //     break;

        // case UART_PORT1:
        //     LPC_SC->PCONP       |=  (1 << 4);
        //     LPC_SC->PCLKSEL0    &= ~(3 << 8);
        //     LPC_SC->PCLKSEL0    |=  (1 << 8);
        //     LPC_PINCON->PINSEL0 &= ~(3 << 30);  // TXD1 is PINSEL0
        //     LPC_PINCON->PINSEL1 &= ~(3 <<  0);  // RXD1 is PINSEL1
        //     LPC_PINCON->PINSEL0 |=  (1 << 30);  // Set pins as 01 01
        //     LPC_PINCON->PINSEL0 |=  (1 <<  0);
        //     break;

        case UART_PORT2:
            LPC_SC->PCONP       |=  (1 << 24);
            LPC_SC->PCLKSEL1    &= ~(3 << 16);
            LPC_SC->PCLKSEL1    |=  (1 << 16);
            LPC_PINCON->PINSEL0 &= ~(0xF << 20);
            LPC_PINCON->PINSEL0 |=  (0x5 << 20); // Set pins as 01 01
            break;

        case UART_PORT3:
            // P0.0     P0.1
            LPC_SC->PCONP       |=  (1 << 25);
            LPC_SC->PCLKSEL1    &= ~(3 << 18);
            LPC_SC->PCLKSEL1    |=  (1 << 18);
            LPC_PINCON->PINSEL0 &= ~(0xF << 0);
            LPC_PINCON->PINSEL0 |=  (0xA << 0); // Set pins as 10 10

            // TXD3     RXD3
            // LPC_PINCON->PINSEL9 &= ~(0xF << 24); // Clear values
            // LPC_PINCON->PINSEL9 |=  (0xF << 24); // Set values for UART3 Rx/Tx
            // lpc_pclk(pclk_uart3, clkdiv_1);
            // lpc_pconp(pconp_uart3, true);
            break;
    }

    // Same configuration across all UART, avoid repetition
    // Clear and enable FIFOs
    UartPtr->FCR = ( (1 << 2) | (1 << 1) | (1 << 0) );

    // Enable DLAB before configuration
    UartPtr->LCR = LCR_DLAB_BIT;
    // Set baud rate divisors
    uint16_t baud = (sys_get_cpu_clock() / (16 * baud_rate)) + 0.5;
    UartPtr->DLM  = (baud >> 8);
    UartPtr->DLL  = (baud >> 0);
    // Disable DLAB
    UartPtr->LCR &= ~(LCR_DLAB_BIT);

    // 8-bit character, 1 stop bit, no parity, no break, disable DLAB
    UartPtr->LCR = 0x3;

    // Enable interrupts for RBR, THRE, and RX LSR
    NVIC_EnableIRQ(IRQPtr);
    UartPtr->IER = ( IER_RBR_BIT | IER_THRE_BIT | IER_RX_LSR_BIT );

    printf("Uart %i initialized.\n", Port);
}

bool Uart::TxAvailable()
{
    return ( UartPtr->LSR & LSR_TEMT_BIT );
}

void Uart::SendString(uint8_t *buffer, size_t buffer_size)
{
    // Error buffer not allocated
    if (buffer == NULL) 
    {
        printf("[Uart::SendString] Input buffer null.\n");
        return;
    }

    for (size_t i=0; i<buffer_size; i++) 
    {
        SendByte(buffer[i]);
    }
}

void Uart::SendString(const char *buffer, size_t buffer_size)
{
    // Error buffer not allocated
    if (buffer == NULL) 
    {
        printf("[Uart::SendString] Input buffer null.\n");
        return;
    }

    for (size_t i=0; i<buffer_size; i++) 
    {
        while ( !SendByte(buffer[i]) );
    }
}

bool Uart::SendByte(uint8_t byte)
{
    // Send to queue
    // Blocks
    if ( !xQueueSend(TxQueue, &byte, portMAX_DELAY) ) 
    {
        return false;
    }

    // If TX registers are empty, receive byte from queue and send to TX register
    if ( TxAvailable() ) 
    {
        if ( xQueueReceive(TxQueue, &byte, 0) ) 
        {
            // Put byte into transmitting register
            UartPtr->THR = byte;
        }
    }

    return true;
}

bool Uart::RxAvailable()
{
    return ( UartPtr->LSR & LSR_RDR_BIT );
}

size_t Uart::ReceiveString(uint8_t *buffer, size_t buffer_size)
{
    if (buffer == NULL) 
    {
        printf("[Uart::ReceiveString] Input buffer null.\n");
        return 0;
    }

    unsigned int index = uxQueueMessagesWaiting(RxQueue);

    uint8_t byte;

    for (unsigned int i=0; i<index; i++) 
    {
        // Blocks
        ReceiveByte(&byte, 0);
        if (i < buffer_size) 
        {
            buffer[i] = byte;
        }
    }

    // If no null-terminating character, add it
    if (index < buffer_size && buffer[index-1] != '\0') 
    {
        buffer[index++] = '\0';
    }

    return (size_t)index;
}

bool Uart::ReceiveByte(uint8_t *byte, uint32_t timeout_ms)
{
    return xQueueReceive(RxQueue, byte, timeout_ms / portTICK_PERIOD_MS);
}