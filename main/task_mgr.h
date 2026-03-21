#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

void task_mgr_register(TaskHandle_t h);
void task_mgr_suspend_all(void);
void task_mgr_resume_all(void);

#ifdef __cplusplus
}
#endif