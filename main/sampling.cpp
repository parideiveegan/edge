#include "sampling.h"
#include "sensor.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "model-parameters/model_metadata.h"

// constants
static constexpr size_t AXES = 3;
static constexpr size_t WINDOW_SAMPLES = EI_CLASSIFIER_RAW_SAMPLE_COUNT;     // 50
static constexpr size_t RING_SAMPLES   = WINDOW_SAMPLES * 2;                 // 100
static constexpr size_t RING_FLOATS    = RING_SAMPLES * AXES;

static constexpr int SAMPLE_PERIOD_MS = 1000 / EI_CLASSIFIER_FREQUENCY;

// filter constants
static constexpr float HP_BETA   = 0.02f;
static constexpr float EMA_ALPHA = 0.10f;
static constexpr float DEADBAND  = 0.01f;

// ring buffer 
static float ring_buf[RING_FLOATS];
static volatile size_t ring_head = 0;
static portMUX_TYPE ring_mux = portMUX_INITIALIZER_UNLOCKED;

static TaskHandle_t infer_task_handle = nullptr;
static volatile bool g_reset_requested = false;

// deadband
static inline float deadband(float v, float th) {
    return (v > -th && v < th) ? 0.0f : v;
}

// called by inference to copy latest 1s (150 floats)
void imu_copy_latest_window(float *dst, size_t len)
{
    const size_t needed = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; // 150
    if (!dst || len < needed) return;

    portENTER_CRITICAL(&ring_mux);

    size_t end_sample = (ring_head + RING_SAMPLES - 1) % RING_SAMPLES;
    size_t start_sample = (end_sample + RING_SAMPLES + 1 - WINDOW_SAMPLES) % RING_SAMPLES;

    for (size_t i = 0; i < WINDOW_SAMPLES; i++) {
        size_t s = (start_sample + i) % RING_SAMPLES;
        size_t src = s * AXES;
        size_t out = i * AXES;
        dst[out + 0] = ring_buf[src + 0];
        dst[out + 1] = ring_buf[src + 1];
        dst[out + 2] = ring_buf[src + 2];
    }

    portEXIT_CRITICAL(&ring_mux);
}

void imu_request_reset(bool enable)
{
    g_reset_requested = enable;
}

void imu_task(void *arg)
{

    infer_task_handle = (TaskHandle_t)arg;

    TickType_t last = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(SAMPLE_PERIOD_MS);

    float gx=0, gy=0, gz=0;
    float fx=0, fy=0, fz=0;
    size_t samples_since_notify = 0;

    while (1) {
        if (g_reset_requested) {
            portENTER_CRITICAL(&ring_mux);
            memset(ring_buf, 0, sizeof(ring_buf));
            ring_head = 0;
            portEXIT_CRITICAL(&ring_mux);

            gx = gy = gz = 0.0f;
            fx = fy = fz = 0.0f;
            samples_since_notify = 0;
            g_reset_requested = false;
        }

        float xr, yr, zr;
        imu_read_g(&xr, &yr, &zr);

        gx = HP_BETA * xr + (1.0f - HP_BETA) * gx;
        gy = HP_BETA * yr + (1.0f - HP_BETA) * gy;
        gz = HP_BETA * zr + (1.0f - HP_BETA) * gz;

        float dx = xr - gx;
        float dy = yr - gy;
        float dz = zr - gz;

        fx = EMA_ALPHA * dx + (1.0f - EMA_ALPHA) * fx;
        fy = EMA_ALPHA * dy + (1.0f - EMA_ALPHA) * fy;
        fz = EMA_ALPHA * dz + (1.0f - EMA_ALPHA) * fz;

        fx = deadband(fx, DEADBAND);
        fy = deadband(fy, DEADBAND);
        fz = deadband(fz, DEADBAND);

        portENTER_CRITICAL(&ring_mux);
        size_t idx = (ring_head % RING_SAMPLES) * AXES;
        ring_buf[idx + 0] = fx;
        ring_buf[idx + 1] = fy;
        ring_buf[idx + 2] = fz;
        ring_head = (ring_head + 1) % RING_SAMPLES;
        portEXIT_CRITICAL(&ring_mux);

        samples_since_notify++;
        if (samples_since_notify >= WINDOW_SAMPLES) {
            samples_since_notify = 0;
            if (infer_task_handle) xTaskNotifyGive(infer_task_handle);
        }

        vTaskDelayUntil(&last, period);
    }
}