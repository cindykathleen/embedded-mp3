#pragma once
#include <freertos/FreeRTOS.h>          // FreeRTOS
#include <freertos/task.h>              // Create tasks

void initialize_wifi(void);

void client_task(void *p);

void server_task(void *p);