/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "command_responder.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "lvgl.h"
#include <cstdio>
#include <cstring>
#include "bsp/esp32_s3_eye.h"
#include "esp_timer.h"

// Ensure you have the display initialized somewhere in your setup
extern "C" void setup_display();

// Debounce mechanism to prevent excessive display updates
static int64_t last_display_update = 0;
static const int64_t MIN_DISPLAY_UPDATE_INTERVAL_US = 500000; // 500ms minimum between updates
static char last_command[32] = "";

// TODO 1: Create styles for different background colors ---------------------
lv_color_t bg_color;
// END TODO 1 ----------------------------------------------------------------
#include "lvgl.h"

static lv_obj_t *g_left = NULL;
static lv_obj_t *g_time = NULL;
static lv_obj_t *g_wifi = NULL;

void ui_set_left_text(lv_obj_t *parent, const char *text)
{
    if (!parent) parent = lv_scr_act();
    if (!parent) return;                 // LVGL not ready -> silently ignore

    if (!g_left) {
        g_left = lv_label_create(parent);
        // set font ONCE here (not every update)
        lv_obj_set_style_text_font(g_left, &lv_font_montserrat_18, 0);
    }

    lv_label_set_text(g_left, text ? text : "");
    lv_obj_align(g_left, LV_ALIGN_TOP_LEFT, 10, 10);
}

void ui_set_time_text(lv_obj_t *parent, const char *hhmm)
{
    if (!parent) parent = lv_scr_act();

    if (!g_time) {
        g_time = lv_label_create(parent);
        lv_obj_set_style_text_font(g_time, &lv_font_montserrat_18, 0);
    }

    lv_label_set_text(g_time, hhmm ? hhmm : "0%");
    lv_obj_align(g_time, LV_ALIGN_TOP_RIGHT, -10, 8);
}

void ui_set_wifi_status(lv_obj_t *parent, bool connected)
{
    if (!parent) parent = lv_scr_act();

    if (!g_wifi) {
        g_wifi = lv_label_create(parent);
        lv_obj_set_style_text_font(g_wifi, &lv_font_montserrat_18, 0);
    }

    lv_label_set_text(g_wifi, connected ? "WiFi: Connected" : "WiFi: Pairing");

    // Align below time (time must exist first)
    if (g_time) {
        lv_obj_align_to(g_wifi, g_left, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
    } else {
        // fallback if time not created yet
        lv_obj_align(g_wifi, LV_ALIGN_TOP_LEFT, 10, 28);
    }
}

void setup_styles()
{
    ui_set_left_text(lv_scr_act(), "12:45");
    ui_set_time_text(lv_scr_act(),"100%");
    ui_set_wifi_status(lv_scr_act(),true);
}

void RespondToCommand(const char *found_command,float score, bool is_new_command)
{
    if (is_new_command)
    {
       // MicroPrintf("Heard %s (%.4f) @%dms", found_command, score, current_time);

        // Check if we should update display (debounce)
        int64_t now = esp_timer_get_time();
        bool should_update_display = false;
        
        // Update display if:
        // 1. Enough time has passed since last update, OR
        // 2. Command has changed
        if ( strcmp(last_command, found_command) != 0 || is_new_command )
        {
            should_update_display = true;
            last_display_update = now;
            strncpy(last_command, found_command, sizeof(last_command) - 1);
            last_command[sizeof(last_command) - 1] = '\0';
        }

        if (!should_update_display)
        {
            // Skip display update to prevent freeze
            printf("skipped....");
            return;
        }

        // Display the recognized command on the LCD
        static lv_obj_t *label = nullptr;
        static lv_obj_t *screen = nullptr;

        bsp_display_lock(10);
        if (label == nullptr)
        {
            screen = lv_scr_act();
            label = lv_label_create(screen);

            // Set label size to avoid overflow
            lv_obj_set_width(label, LV_HOR_RES - 20);

            // Enable word wrap and auto resize
            lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

            // Set text style to increase size
            static lv_style_t style;
            lv_style_init(&style);

            lv_style_set_text_font(&style, &lv_font_montserrat_22);


            lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);
            lv_obj_add_style(label, &style, 0);
        }

        char display_str[128];
        snprintf(display_str, sizeof(display_str), "%s",found_command);

        lv_style_t bg_style;
        lv_style_init(&bg_style);
        
        // Set the text of the label (only if different to minimize updates)
        lv_label_set_text(label, display_str);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0); // Center the label

        // Force display update
        //lv_refr_now(NULL);
        bsp_display_unlock();
    }
}
