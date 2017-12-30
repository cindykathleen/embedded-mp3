// SchedulerTask
#include "tasks.hpp"
// Project libraries
#include "mp3_tasks.hpp"
#include "mp3_uart.hpp"


#define CPP_SCHEDULER 1
#define TERMINAL_TASK 1

// No tasks need input parameters or handles, so simplify with a macro
#define CREATE_TASK_LOW(pointer, stack)  (xTaskCreate(pointer, #pointer, stack, NULL, PRIORITY_LOW,    NULL))
#define CREATE_TASK_MED(pointer, stack)  (xTaskCreate(pointer, #pointer, stack, NULL, PRIORITY_MEDIUM, NULL))
#define CREATE_TASK_HIGH(pointer, stack) (xTaskCreate(pointer, #pointer, stack, NULL, PRIORITY_HIGH,   NULL))

int main(void)
{
    // All initialization functions in order
    // Init_Uart();            // UART driver, queues
    Init_ButtonTask();      // Queues, GPIOs, interrupts
    Init_DecoderTask();     // Decoder driver, track list, SD card, semaphores
    Init_LCDTask();         // LCD driver, screen, mutex
    // Init_TxTask();          // Queue
    // Init_RxTask();          // Queue

    // Low priority tasks
    CREATE_TASK_LOW(DecoderTask    , 4096);    // Regular operation
    // CREATE_TASK_LOW(TxTask         , 1024);    // Regular operation

    // Medium priority tasks
    CREATE_TASK_MED(ButtonTask     , 1024);    // Serviced immediately from GPIO interrupt
    CREATE_TASK_MED(LCDTask        , 1024);    // Serviced immediately when receiving a queue from ButtonTask 
    // CREATE_TASK_MED(RxTask         , 1024);    // Serviced immediately from UART interrupt

    // High priority tasks
    // CREATE_TASK_HIGH(DMATask,       1024);
    // CREATE_TASK_HIGH(WatchdogTask,  1024);

#if CPP_SCHEDULER

  #if TERMINAL_TASK
    scheduler_add_task(new terminalTask(PRIORITY_HIGH));
  #endif

    scheduler_start();

#else

    vTaskStartScheduler();
    
#endif
    
    return -1;
}