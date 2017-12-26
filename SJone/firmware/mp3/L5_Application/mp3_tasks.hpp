#pragma once
// Project libraries
#include "common.hpp"


/*///////////////////////////////////////////////////
 * File dependency structure:                       *
 *                                                  *
 *      msg_protocol        utilities               *
 *              \            /                      *
 *                  common                          *
 *                  /    \                          *
 *           TxTask        mp3_struct               *
 *           RxTask                 \               *
 *                                     DecoderTask  *
 *                                     LCDTask      *
 *                                                  *
 *////////////////////////////////////////////////////

/**
 *  ESP32 --> UART --> ESP32Task --> MessageRxQueue --> MP3Task
 *  MP3Task --> MessageTxQueue --> ESP32Task --> UART --> ESP32
 */

// @description : Initializes ButtonTask
//                1. Initializes semaphores and queues
//                2. Initializes GPIOs and interrupts
void Init_ButtonTask(void);

// @description : Task for responding to GPIO interrupts from external buttons
//                1. Receives a GPIO interrupt
//                2. Receives the GPIO number over a queue from the interrupt
//                3. Sends the same number over a queue to DecoderTask
//                The point for a pass through between 2 queues is because the interrupt will trigger
//                this low priority task, which will signal a medium priority task.  This way other tasks will never
//                waste time on worrying about the GPIOs.
void ButtonTask(void *p);


// @description : Initializes DecoderTask
//                1. Initializes SPI 0
//                2. Initializes the decoder
//                3. Initializes the tracklist
void Init_DecoderTask(void);

// @description : Task for communicating with the VS1053b device
//                1. Checks ButtonQueue for any GPIO triggers and services them by changing state or a decoder setting
//                2. Executes the next state in the state machine
//                3. Checks for any command messages in MessageRxQueue // TODO : move up?
void DecoderTask(void *p);


// @description : Initializes TxTask
//                1. Creates MessageTxQueue
void Init_TxTask(void);

// @description : Task for sending diagnostic messages to the ESP32
//                1. Blocks until receives a message from MessageTxQueue
//                2. Sends the message serially over UART
void TxTask(void *p);


// @description : Initializes RxTask
//                1. Creates MessageRxQueue
void Init_RxTask(void);

// @description : Task for receiving command messages from the ESP32
//                1. Blocks until receives a byte from UartRxQueue from a uart interrupt
//                2. Sends the message to MessageRxQueue
void RxTask(void *p);


// @description : This task monitors responses from other tasks to ensure none of them are stuck
//                in an undesired state.  Periodically checks once per second.
void WatchdogTask(void *p);

void LCDTask(void *p);