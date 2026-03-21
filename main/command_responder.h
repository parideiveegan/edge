#ifndef TENSORFLOW_LITE_MICRO_EXAMPLES_MICRO_SPEECH_COMMAND_RESPONDER_H_
#define TENSORFLOW_LITE_MICRO_EXAMPLES_MICRO_SPEECH_COMMAND_RESPONDER_H_

#include "tensorflow/lite/c/common.h"
#include <cstdint>
#include "lvgl.h"


void RespondToCommand(const char* found_command,float score, bool is_new_command);

void setup_styles();
void ui_set_left_text(lv_obj_t *parent, const char *text);
void ui_set_time_text(lv_obj_t *parent, const char *hhmm);
void ui_set_wifi_status(lv_obj_t *parent, bool connected);

#endif 