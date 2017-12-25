#include "tasks.hpp"
#include "mp3_tasks.hpp"

#define CPP_SCHEDULER 1
#define TERMINAL_TASK 1

int main(void)
{
    Init_ButtonTask();
    Init_DecoderTask();

    xTaskCreate(ButtonTask,   "ButtonTask",   1024,  NULL, PRIORITY_LOW,    NULL);
    xTaskCreate(DecoderTask,  "DecoderTask",  4096,  NULL, PRIORITY_MEDIUM, NULL);
    // xTaskCreate(WatchdogTask, "WatchdogTask", 256,  NULL, PRIORITY_HIGH,   NULL);
    // xTaskCreate(TxTask,       "TxTask",       1024, NULL, PRIORITY_MEDIUM, NULL);
    // xTaskCreate(RxTask,       "RxTask",       1024, NULL, PRIORITY_MEDIUM, NULL);
    // xTaskCreate(LCDTask,      "LCDTask",      2048, NULL, PRIORITY_HIGH, NULL);

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