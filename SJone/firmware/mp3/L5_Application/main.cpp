#include "tasks.hpp"
#include "mp3_tasks.hpp"


int main(void)
{    
    xTaskCreate(DecoderTask,  "DecoderTask",  5000, NULL, PRIORITY_MEDIUM, NULL);
    xTaskCreate(WatchdogTask, "WatchdogTask", 256,  NULL, PRIORITY_HIGH,   NULL);
    xTaskCreate(TxTask,       "TxTask",       2048, NULL, PRIORITY_MEDIUM, NULL);
    xTaskCreate(RxTask,       "RxTask",       2048, NULL, PRIORITY_MEDIUM, NULL);

    scheduler_add_task(new terminalTask(PRIORITY_HIGH));
    scheduler_start();
    return -1;
}