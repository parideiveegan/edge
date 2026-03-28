#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sensor.h"
#include "sampling.h"
#include "inference.h"
#include "command_responder.h"
#include "bsp/esp32_s3_eye.h"
#include "esp_wifi.h"
#include "sleep.h"
#include "wifi_sta.h"
#include "task_mgr.h"


#define REG_WHO_AM_I        0x00
#define REG_PWR_CTRL        0x11

// ---------------- app_main ----------------
extern "C" void app_main(void)
{
    i2c_master_init();

    bsp_display_start();
    bsp_display_backlight_on();
    setup_styles();
    up_sleep_init();
    up_sleep_task_start();
    

    vTaskDelay(pdMS_TO_TICKS(500));
    wifi_init_sta();

    
    uint8_t who = 0;
    qma_read(REG_WHO_AM_I, &who, 1);
    if (who != 0xE7) {
        printf("sensor not detected (WHO_AM_I=0x%02X)\n", who);
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }


    ESP_ERROR_CHECK(qma_write(REG_PWR_CTRL, 0x80));
    vTaskDelay(pdMS_TO_TICKS(100));

    TaskHandle_t infer_handle = NULL;
    TaskHandle_t imu_handle = NULL;
    xTaskCreatePinnedToCore(inference_task, "infer_task", 8192, NULL, 4,  &infer_handle, 1);
    xTaskCreatePinnedToCore(imu_task, "imu_task", 4096, (void*)infer_handle, 5, &imu_handle, 0);
    task_mgr_register(infer_handle);
    task_mgr_register(imu_handle);
    
}