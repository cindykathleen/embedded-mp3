// #pragma once
// #include "FreeRTOS.h"
// #include "LPC17xx.h"
// #include "singleton_template.hpp"

// #define DEFAULT_BAUDRATE (9600)
// // Interrupt Enable Bits
// #define IER_RBR_BIT     (1 << 0)    // RBR interrupt enable
// #define IER_THRE_BIT    (1 << 1)    // THRE interrupt enable
// #define IER_RX_LSR_BIT  (1 << 2)    // RX line status interrupt enable
// // Interrupt Status Bits
// #define IIR_RX_LSR_BIT  (0x3 << 1)  // RX line status error
// #define IIR_RXREADY_BIT (1   << 2)  // RX data available
// #define IIR_TIO_BIT     (0x3 << 2)  // Character time out indication
// #define IIR_THRE_BIT    (1   << 1)  // THRE
// // LCR Divisor Latch Access Bit
// #define LCR_DLAB_BIT    (1 << 7)    // Disable before configuration
// // Line Status Register bits
// #define LSR_RDR_BIT     (1 << 0)    // Set when RX FIFO is not empty
// #define LSR_OE_BIT      (1 << 1)    // Overrun flag, means new data is lost
// #define LSR_PE_BIT      (1 << 2)    // Parity error
// #define LSR_FE_BIT      (1 << 3)    // Framing error, incorrect stop bit, unsynchronized
// #define LSR_BI_BIT      (1 << 4)    // Break interrupt
// #define LSR_THRE_BIT    (1 << 5)    // Transmit holding register empty
// #define LSR_TEMT_BIT    (1 << 6)    // Both THR and TSR (Transmit Shift Register) are empty
// #define LSR_RXFE_BIT    (1 << 7)    // RBR contains an error (framing, parity, break interrupt)


// // Not using UART0 or UART1
// typedef enum 
// {
//     UART_PORT2 = 2, 
//     UART_PORT3 = 3 
// } uart_port_E;

// typedef enum 
// { 
//     POLLING, 
//     INTERRUPT 
// } uart_mode_E;

// extern "C"
// {
//     void UART2_IRQHandler();
//     void UART3_IRQHandler();
// }

// class Uart
// {
// public:

//     // UART0: TX: P0.2   |  RX: P0.3
//     // UART2: TX: P0.10  |  RX: P0.11
//     // UART3: TX: P0.0   |  RX: P0.1

//     // Initializes registers and configures baud rate
//     void    Init(uint32_t baud_rate=DEFAULT_BAUDRATE);

//     // Checks if TX buffer is empty
//     bool    TxAvailable();

//     // Send string
//     // @param buffer        : pre-allocated buffer
//     // @param buffer_size   : size of pre-allocated buffer
//     void    SendString(uint8_t *buffer, size_t buffer_size);
//     void    SendString(const char *buffer, size_t buffer_size);

//     // Send byte
//     bool    SendByte(uint8_t byte);

//     // Checks if anything waiting in RX buffer
//     bool    RxAvailable();

//     // Receive string
//     // @param buffer        : pre-allocated buffer
//     // @param buffer_size   : size of pre-allocated buffer
//     // @return              : size of buffer filled
//     size_t  ReceiveString(uint8_t *buffer, size_t buffer_size);

//     // Receive byte
//     bool    ReceiveByte(uint8_t *byte, uint32_t timeout_ms);

// protected:

//     // Constructor
//     Uart(uart_port_E port);

//     // Member variables
//     uart_port_E      Port;
//     LPC_UART_TypeDef *UartPtr;
//     IRQn_Type        IRQPtr;
// };

// class Uart2 : public Uart, public SingletonTemplate <Uart2>
// {
// private:

//     Uart2() : Uart(UART_PORT2)
//     {
//         // Empty
//     }

//     friend class SingletonTemplate <Uart2>;
// };

// class Uart3 : public Uart, public SingletonTemplate <Uart3>
// {
// private:

//     Uart3() : Uart(UART_PORT3)
//     {
//         // Empty
//     }

//     friend class SingletonTemplate <Uart3>;
// };

// /*///////////////////////////////////////////////////////////////////////////////////////////////////
//  *   [RBR] Receiver Buffer Register     : Contains the next received character to be read           *
//  *   [THR] Transmit Holding Register    : Contains the next to be transmitted character             *
//  *   [DLL] Divisor Latch LSB            : LSB of baud rate divisor value                            *
//  *   [DLM] Divisor Latch MSB            : MSB of baud rate divisor value                            *
//  *   [IER] Interrupt Enable Register    : Contains enable bits for UART interrupts                  *
//  *   [IIR] Interrupt ID Register        : Identifies which interrupts are pending                   *
//  *   [FCR] FIFO Control Register        : Controls UART FIFO usage and modes                        *
//  *   [LCR] Line Control Register        : Contains controls for frame formatting + break generation *
//  *   [LSR] Line Status Register         : Contains flags for tx/rx status, line errors              *
//  *   [SCR] Scratch Pad Register         : 8-bit temp storage for software                           *
//  *   [ACR] Auto-baud Control Register   : Contains controls for auto-baud feature                   *
//  *   [ICR] IrDA Control Register        : Enables/configures IrDA mode                              *
//  *   [FDR] Fractional Divider Register  : Generates clock input for baud rate divider               *
//  *   [TER] Transmit Enable Register     : Turns off UART transmitter for use with software control  *
//  *////////////////////////////////////////////////////////////////////////////////////////////////////