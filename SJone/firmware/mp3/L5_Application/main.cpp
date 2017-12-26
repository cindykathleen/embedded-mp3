// SchedulerTask
#include "tasks.hpp"
// Project libraries
#include "mp3_tasks.hpp"
#include "mp3_uart.hpp"


#define CPP_SCHEDULER 1
#define TERMINAL_TASK 1

// No tasks need input parameters or handles, so simplify with a macro
#define CREATE_TASK_LOW(pointer, stack)  (xTaskCreate(pointer, "pointer", stack, NULL, PRIORITY_LOW,    NULL))
#define CREATE_TASK_MED(pointer, stack)  (xTaskCreate(pointer, "pointer", stack, NULL, PRIORITY_MEDIUM, NULL))
#define CREATE_TASK_HIGH(pointer, stack) (xTaskCreate(pointer, "pointer", stack, NULL, PRIORITY_HIGH,   NULL))

int main(void)
{
    Init_Uart();
    Init_ButtonTask();
    Init_DecoderTask();
    Init_TxTask();
    Init_RxTask();

    CREATE_TASK_LOW(ButtonTask,  1024);

    CREATE_TASK_MED(DecoderTask, 4096);
    CREATE_TASK_MED(TxTask,      1024);
    CREATE_TASK_MED(RxTask,      1024);

    // xTaskCreate(ButtonTask,    "ButtonTask",    1024,   NULL,   PRIORITY_LOW,      NULL);
    // xTaskCreate(DecoderTask,   "DecoderTask",   4096,   NULL,   PRIORITY_MEDIUM,   NULL);
    // xTaskCreate(TxTask,        "TxTask",        1024,   NULL,   PRIORITY_MEDIUM,   NULL);
    // xTaskCreate(RxTask,        "RxTask",        1024,   NULL,   PRIORITY_MEDIUM,   NULL);
    // xTaskCreate(LCDTask,       "LCDTask",       2048,   NULL,   PRIORITY_HIGH,     NULL);
    // xTaskCreate(WatchdogTask,  "WatchdogTask",  256,    NULL,   PRIORITY_HIGH,     NULL);

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