#include "sleep.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "bsp/esp32_s3_eye.h"
#include "task_mgr.h"
#include "esp_wifi.h"

static const char *TAG = "UP_SLEEP";

// ---- Board schematic: Button_Array_ADC -> GPIO1 -> ADC1_CH0 ----
#define BTN_ADC_UNIT   ADC_UNIT_1
#define BTN_ADC_CH     ADC_CHANNEL_0      // ADC1_CH0 (GPIO1)
#define BTN_ADC_ATTEN  ADC_ATTEN_DB_11

// ---- Wake source: BOOT -> GPIO0 ----
#define WAKE_GPIO      GPIO_NUM_0

// ---- CALIBRATE THIS ----
// Press UP+ and note the raw ADC value, then set min/max around it.
// Start with a wide range, then narrow.
static int UP_RAW_MIN = 300;   // example
static int UP_RAW_MAX = 900;   // example

static adc_oneshot_unit_handle_t s_adc = NULL;

static void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = BTN_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc));

    adc_oneshot_chan_cfg_t ch_cfg = {
        .atten = BTN_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, BTN_ADC_CH, &ch_cfg));
}

static int adc_read_raw(void)
{
    int raw = 0;
    adc_oneshot_read(s_adc, BTN_ADC_CH, &raw);
    return raw;
}

static void wake_gpio0_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << WAKE_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);

    ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());
    ESP_ERROR_CHECK(gpio_wakeup_enable(WAKE_GPIO, GPIO_INTR_LOW_LEVEL)); // BOOT press pulls low
}

static void wait_up_release(void)
{
    // simple debounce + ensure button released before sleeping
    vTaskDelay(pdMS_TO_TICKS(80));
    for (int i = 0; i < 60; i++) {
        int raw = adc_read_raw();
        bool pressed = (raw >= UP_RAW_MIN && raw <= UP_RAW_MAX);
        if (!pressed) return;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void up_sleep_task(void *arg)
{
    (void)arg;
    wake_gpio0_init();

    ESP_LOGI(TAG, "UP+ -> light sleep | Wake using BOOT(GPIO0)");

    while (1) {
        int raw = adc_read_raw();
        bool up_pressed = (raw >= UP_RAW_MIN && raw <= UP_RAW_MAX);

        if (up_pressed) {
            ESP_LOGI(TAG, "UP+ pressed (raw=%d). Going to light sleep...", raw);

            wait_up_release();

            ESP_ERROR_CHECK(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL));
            ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());
            ESP_ERROR_CHECK(gpio_wakeup_enable(GPIO_NUM_0, GPIO_INTR_LOW_LEVEL));
            task_mgr_suspend_all();
            esp_wifi_disconnect();
            esp_wifi_stop();
            esp_light_sleep_start();   // returns after BOOT press
            
            
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void up_sleep_init(void)
{
    adc_init();
}

void up_sleep_task_start(void)
{
    xTaskCreate(up_sleep_task, "up_sleep", 4096, NULL, 3, NULL);
}