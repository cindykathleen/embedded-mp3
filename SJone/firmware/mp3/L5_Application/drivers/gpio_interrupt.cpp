// #include "gpio_interrupt.hpp"

// // Global variables
// SemaphoreHandle_t Sem1 = xSemaphoreCreateBinary();
// SemaphoreHandle_t Sem2 = xSemaphoreCreateBinary();
// // Initialize pointer to array of pointers, pointing to structs, initialized NULL
// static gpio_interrupt_t **gpio_port0_interrupts;
// static gpio_interrupt_t **gpio_port2_interrupts;
// static bool initialized = false;

// #if 0
// // Currently only one callback for rising + falling, can separate later
// extern "C" 
// {
//     void EINT2_IRQHandler()
//     {
//         // Clear all active interrupts for GPIO port 0
//         for (int i=0; i<31; i++) 
//         {
//             // Reserved bits [14:12]
//             if (i >= 12 && i <=14) 
//             {
//                 continue;
//             }
//             // Checks both edges
//             if ( (LPC_GPIOINT->IO0IntStatR & (1 << i)) || (LPC_GPIOINT->IO0IntStatF & (1 << i)) ) 
//             {
//                 LPC_GPIOINT->IO0IntClr  |= (1 << i);
//                 LPC_GPIO0->FIOCLR       |= (1 << i);
//                 // If gpio interrupt handler exists call callback
//                 if (gpio_port0_interrupts[i] != NULL) 
//                 {
//                     gpio_port0_interrupts[i]->callback();
//                 }
//             }
//         }

//         // Clear all active interrupts for GPIO port 2
//         for (int i=0; i<14; i++) 
//         {
//             // Check both edges
//             if ( (LPC_GPIOINT->IO2IntStatR & (1 << i)) || (LPC_GPIOINT->IO2IntStatF & (1 << i)) ) 
//             {
//                 LPC_GPIOINT->IO2IntClr  |= (1 << i);
//                 LPC_GPIO2->FIOCLR       |= (1 << i);
//                 // If gpio interrupt handler exists call callback
//                 if (gpio_port2_interrupts[i] != NULL) 
//                 {
//                     gpio_port2_interrupts[i]->callback();
//                 }
//             }
//         }
        
//         LedsToggleAll();
//     }

//     void EINT3_IRQHandler()
//     {
//         // Clear all active interrupts for GPIO port 0
//         for (int i=0; i<31; i++) 
//         {
//             // Reserved bits [14:12]
//             if (i >= 12 && i <=14) 
//             {
//                 continue;
//             }
//             // Check both edges
//             if ( (LPC_GPIOINT->IO0IntStatR & (1 << i)) || (LPC_GPIOINT->IO0IntStatF & (1 << i)) ) 
//             {
//                 LPC_GPIOINT->IO0IntClr  |= (1 << i);
//                 LPC_GPIO0->FIOCLR       |= (1 << i);
//                 // If gpio interrupt handler exists call callback
//                 if (gpio_port0_interrupts[i] != NULL) 
//                 {
//                     gpio_port0_interrupts[i]->callback();
//                 }
//             }
//         }

//         // Clear all active interrupts for GPIO port 2
//         for (int i=0; i<14; i++) 
//         {
//             // Check both edges
//             if ( (LPC_GPIOINT->IO2IntStatR & (1 << i)) || (LPC_GPIOINT->IO2IntStatF & (1 << i)) ) 
//             {
//                 LPC_GPIOINT->IO2IntClr  |= (1 << i);
//                 LPC_GPIO2->FIOCLR       |= (1 << i);
//                 // If gpio interrupt handler exists call callback
//                 if (gpio_port2_interrupts[i] != NULL) 
//                 {
//                     gpio_port2_interrupts[i]->callback();
//                 }
//             }
//         }
        
//         LedsToggleAll();
//     }
// }
// #endif

// ///////////////////////////////////////////////////////////////////////////////////////////////////

// static void initialize_interrupt(gpio_interrupt_t *interrupt)
// {
//     // Disable interrupt before configuration
//     switch (interrupt->eint)
//     {
//         case EINT0: NVIC_DisableIRQ(EINT0_IRQn); break;
//         case EINT1: NVIC_DisableIRQ(EINT1_IRQn); break;
//         case EINT2: NVIC_DisableIRQ(EINT2_IRQn); break;
//         case EINT3: NVIC_DisableIRQ(EINT3_IRQn); break;
//     }

//     // Interrupt configuration
//     switch (interrupt->eint)
//     {
//         case EINT0: 
//             LPC_SC->EXTMODE  |= (1 << 0);   // Edge sensitive
//             LPC_SC->EXTPOLAR |= (1 << 0);   // Rising edge
//             LPC_SC->EXTINT   |= (1 << 0);   // Clear EINT bit to start
//             break;
//         case EINT1: 
//             LPC_SC->EXTMODE  |= (1 << 1);
//             LPC_SC->EXTPOLAR |= (1 << 1);
//             LPC_SC->EXTINT   |= (1 << 1);
//             break;
//         case EINT2: 
//             LPC_SC->EXTMODE  |= (1 << 2);
//             LPC_SC->EXTPOLAR |= (1 << 2);
//             LPC_SC->EXTINT   |= (1 << 2);
//             break;
//         case EINT3: 
//             LPC_SC->EXTMODE  |= (1 << 3);
//             LPC_SC->EXTPOLAR |= (1 << 3);
//             LPC_SC->EXTINT   |= (1 << 3);
//             break;
//     }

//     // Turn on rising edge interrupt for pin
//     switch (interrupt->port)
//     {
//         case GPIO_PORT0:

//             (interrupt->edge == RISING) ?
//                 ( LPC_GPIOINT->IO0IntEnR |= (1 << interrupt->pin) ) :
//                 ( LPC_GPIOINT->IO0IntEnF |= (1 << interrupt->pin) );
//             break;

//         case GPIO_PORT2:

//             (interrupt->edge == RISING) ?
//                 ( LPC_GPIOINT->IO2IntEnR |= (1 << interrupt->pin) ) :
//                 ( LPC_GPIOINT->IO2IntEnF |= (1 << interrupt->pin) );
//             break;

//         default:
//             printf("Interrupt on GPIO ports other than 0 and 2 are not supported.\n");
//             exit(EXIT_FAILURE);
//     }

//     // Enable interrupt
//     switch (interrupt->eint)
//     {
//         case EINT0: NVIC_EnableIRQ(EINT0_IRQn); break;
//         case EINT1: NVIC_EnableIRQ(EINT1_IRQn); break;
//         case EINT2: NVIC_EnableIRQ(EINT2_IRQn); break;
//         case EINT3: NVIC_EnableIRQ(EINT3_IRQn); break;
//     }
// }

// void initialize_gpio_interrupt_arrays()
// {
//     gpio_port0_interrupts = new gpio_interrupt_t*[31]();
//     gpio_port2_interrupts = new gpio_interrupt_t*[14]();

//     for (int i=0; i<31; i++) {
//         gpio_port0_interrupts[i] = NULL;
//     }
//     for (int i=0; i<14; i++) {
//         gpio_port2_interrupts[i] = NULL;
//     }

//     initialized = true;
// }

// void install_gpio_interrupt(external_interrupt_t  eint,
//                             interrupt_edge_t      edge,
//                             gpio_port_t           port,
//                             gpio_pin_t            pin,
//                             void_function_ptr_t   callback)
// {
//     if (!initialized)
//     {
//         printf("[ERROR] Interrupt callback array not initialized!\n");
//         exit(EXIT_FAILURE);
//     }

//     gpio_interrupt_t *interrupt = new gpio_interrupt_t;
//     // Store configuration
//     interrupt->eint     = eint;
//     interrupt->edge     = edge;
//     interrupt->port     = port;
//     interrupt->pin      = pin;
//     interrupt->callback = callback;

//     // Store callback
//     switch (interrupt->port)
//     {
//         case GPIO_PORT0: gpio_port0_interrupts[interrupt->pin] = interrupt; break;
//         case GPIO_PORT2: gpio_port2_interrupts[interrupt->pin] = interrupt; break;
//         default: 
//             printf("[ERROR] Interrupt on GPIO ports other than 0 and 2 are not supported.\n");
//             exit(EXIT_FAILURE);
//     }

//     // Initialize
//     initialize_interrupt(interrupt);

//     printf("GPIO External Interrupt initialized on port: %i pin: %i\n", 
//                                                 interrupt->port, 
//                                                 interrupt->pin);
// }

// void install_gpio_interrupt(gpio_interrupt_t *interrupt)
// {
//     if (!initialized)
//     {
//         printf("[ERROR] Interrupt callback array not initialized!\n");
//         exit(EXIT_FAILURE);
//     }

//     // Store callback
//     switch (interrupt->port)
//     {
//         case GPIO_PORT0: gpio_port0_interrupts[interrupt->pin] = interrupt; break;
//         case GPIO_PORT2: gpio_port2_interrupts[interrupt->pin] = interrupt; break;
//         default: 
//             printf("[ERROR] Interrupt on GPIO ports other than 0 and 2 are not supported.\n");
//             exit(EXIT_FAILURE);
//     }

//     // Initialize
//     initialize_interrupt(interrupt);

//     printf("GPIO External Interrupt initialized on port: %i pin: %i\n", 
//                                                 interrupt->port, 
//                                                 interrupt->pin);
// }