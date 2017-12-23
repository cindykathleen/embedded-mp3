// #pragma once
// #include <stdio.h>
// #include <cstdlib>
// #include <cstring>
// #include <FreeRTOS.h>
// #include <semphr.h>
// #include "L5_Application/drivers/led.hpp"
// #include "L5_Application/drivers/gpio.hpp"

// typedef enum { EINT0, EINT1, EINT2, EINT3 } external_interrupt_t;
// typedef enum { RISING, FALLING }            interrupt_edge_t;
// typedef void (*void_function_ptr_t)();

// // Configuration for a GPIO interrupt
// typedef struct gpio_interrupt_struct
// {
//     external_interrupt_t    eint;
//     interrupt_edge_t        edge;
//     gpio_port_t             port;
//     gpio_pin_t              pin;
//     void_function_ptr_t     callback;

//     gpio_interrupt_struct()
//     {
//         eint     = EINT3;
//         edge     = RISING;
//         port     = GPIO_PORT0;
//         pin      = 0;
//         callback = NULL;
//     }

// } gpio_interrupt_t;

// // Global variables
// extern SemaphoreHandle_t Sem1;
// extern SemaphoreHandle_t Sem2;

// #if 0
// // Interrupt Handlers
// extern "C"
// {
//     void EINT2_IRQHandler();
//     void EINT3_IRQHandler();
// }
// #endif

// // Initializes the arrays, allocates, and set to NULL
// void initialize_gpio_interrupt_arrays();

// // Install with separate parameters
// void install_gpio_interrupt(external_interrupt_t  eint, 
//                             interrupt_edge_t      edge,
//                             gpio_port_t           port, 
//                             gpio_pin_t            pin,
//                             void_function_ptr_t   callback);

// // Install with a single struct
// void install_gpio_interrupt(gpio_interrupt_t *interrupt);