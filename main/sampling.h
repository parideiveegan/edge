#pragma once
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// keep your task name
void imu_task(void *arg);

// inference needs this
void imu_copy_latest_window(float *dst, size_t len);

// optional reset trigger (Option2)
void imu_request_reset(bool enable);

#ifdef __cplusplus
}
#endif