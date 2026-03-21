#include "task_mgr.h"
#include "bsp/esp32_s3_eye.h"
#include "esp_wifi.h"
#define MAX_TASKS 12

static TaskHandle_t s_tasks[MAX_TASKS];
static int s_count = 0;

void task_mgr_register(TaskHandle_t h)
{
    if (!h) return;
    if (s_count >= MAX_TASKS) return;

    // prevent duplicates
    for (int i = 0; i < s_count; i++) {
        if (s_tasks[i] == h) return;
    }
    s_tasks[s_count++] = h;
}

void task_mgr_suspend_all(void)
{
    TaskHandle_t self = xTaskGetCurrentTaskHandle();
    bsp_display_enter_sleep();
    bsp_display_brightness_set(0);
    for (int i = 0; i < s_count; i++) {
        TaskHandle_t h = s_tasks[i];
        if (!h) continue;
        if (h == self) continue;      // don't suspend the caller
        vTaskSuspend(h);
    }
}

void task_mgr_resume_all(void)
{
    bsp_display_exit_sleep();
    bsp_display_brightness_set(20);

    for (int i = 0; i < s_count; i++) {
        TaskHandle_t h = s_tasks[i];
        if (!h) continue;
        vTaskResume(h);
    }
}