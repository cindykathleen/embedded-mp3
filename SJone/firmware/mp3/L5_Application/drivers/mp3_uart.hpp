#pragma once
// Project libraries
#include "common.hpp"


extern "C"
{
    void uart_interupt_callback(LPC_UART_TypeDef *uart_ptr);

    void UART3_IRQHandler();
}

void Init_Uart(void);

bool uart_rx_byte(uint8_t &byte, uint32_t timeout_ms);

bool uart_tx_byte(uint8_t byte, TickType_t timeout_ms);