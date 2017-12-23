#include "tasks.hpp"
#include "mp3_tasks.hpp"

#define CPP_SCHEDULER 1

int main(void)
{
    xTaskCreate(ButtonTask,   "ButtonTask",   256,  NULL, PRIORITY_HIGH, NULL);
    xTaskCreate(DecoderTask,  "DecoderTask",  4098, NULL, PRIORITY_HIGH, NULL);
    // xTaskCreate(WatchdogTask, "WatchdogTask", 256,  NULL, PRIORITY_HIGH,   NULL);
    // xTaskCreate(TxTask,       "TxTask",       1024, NULL, PRIORITY_MEDIUM, NULL);
    // xTaskCreate(RxTask,       "RxTask",       1024, NULL, PRIORITY_MEDIUM, NULL);
    // xTaskCreate(LCDTask,      "LCDTask",      2048, NULL, PRIORITY_HIGH, NULL);

#if CPP_SCHEDULER
    scheduler_add_task(new terminalTask(PRIORITY_HIGH));
    scheduler_start();
#else
    vTaskStartScheduler();
#endif
    
    return -1;
}