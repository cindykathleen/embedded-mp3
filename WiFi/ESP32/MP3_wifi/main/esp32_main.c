/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "scan.c"
#include "packet_filter.h"
                                            

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();                                                                                                                                                                   
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // connect to wifi
    wifi_scan();
    uart_init();

    // task create of command task, command uart
    // task create of diag task, diag uart
    xTaskCreate(command_task,           "esp32_data_command",    4096, NULL, 1, NULL);
    //xTaskCreate(command_send_uart_data, "esp32_uart_command",    1024, NULL, 3, NULL);

}
