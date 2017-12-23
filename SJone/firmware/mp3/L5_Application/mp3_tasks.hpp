#pragma once
#include <stdarg.h>
#include "common.hpp"
#include "vs1053b.hpp"


extern SemaphoreHandle_t ButtonSemaphores[5];

void ButtonTask(void *p);

void LCDTask(void *p);

// @description : Task for communicating with the VS1053b device
// @priority    : PRIORITY_HIGH / PRIORITY_MED?
void DecoderTask(void *p);

/**
 *  ESP32 --> UART --> ESP32Task --> MessageRxQueue --> MP3Task
 *  MP3Task --> MessageTxQueue --> ESP32Task --> UART --> ESP32
 */

// @description : Task for sending diagnostic messages to the ESP32
//                1. Receives a message from MessageTxQueue
//                2. Sends the message serially over UART
// @priority    : PRIORITY_MED
void TxTask(void *p);

// @description : Task for receiving command messages from the ESP32
//                1. Reads a message serially over UART
//                2. Sends the message to MessageRxQueue
// @priority    : PRIORITY_MED
void RxTask(void *p);

// @description : This task monitors responses from other tasks to ensure none of them are stuck
//                in an undesired state.  Periodically checks once per second.
// @priority    : PRIORITY_HIGH
void WatchdogTask(void *p);