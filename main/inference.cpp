#include "inference.h"
#include "sampling.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "command_responder.h"
#include "udp_client.h"
#include "wifi_sta.h"

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

// inference buffer
static float infer_buf[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
static float *g_infer_ptr = infer_buf;


int ei_get_data(size_t offset, size_t length, float *out_ptr)
{
    if (offset + length > EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) return -1;
    memcpy(out_ptr, g_infer_ptr + offset, length * sizeof(float));
    return 0;
}


void copy_latest_window(void)
{
    imu_copy_latest_window(infer_buf, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    g_infer_ptr = infer_buf;
}

void inference_task(void *arg)
{
    
    (void)arg;
    
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        copy_latest_window();

        signal_t signal;
        signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
        signal.get_data = ei_get_data;

        ei_impulse_result_t result;
        memset(&result, 0, sizeof(result));

        EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
        if (res != EI_IMPULSE_OK) {
            printf("EI error: %d\n", res);
            continue;
        }

        size_t best_i = 0;
        float best_v = result.classification[0].value;
        for (size_t i = 1; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            if (result.classification[i].value > best_v) {
                best_v = result.classification[i].value;
                best_i = i;
            }
        }
       
        // reset after prediction
        int is_none = strcmp(result.classification[best_i].label , "none");
        if (is_none !=0 && best_v > 0.80f){
            RespondToCommand(result.classification[best_i].label,best_v, true);
             printf("%s,%.2f\n", result.classification[best_i].label, best_v);
            
             if (wifi_is_connected()){
                udp_send_command(result.classification[best_i].label);                
            }
            imu_request_reset(true);
        }
    }
}