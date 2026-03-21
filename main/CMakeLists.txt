idf_component_register(
    SRCS 
        "main.cpp"
        "sensor.cpp"
        "sampling.cpp"
        "inference.cpp"
        "command_responder.cc"
        "wifi_sta.cpp"
        "udp_client.cpp"
        "sleep.cpp"
        "task_mgr.cpp"
        ""
    PRIV_REQUIRES esp_driver_i2s
    INCLUDE_DIRS "."
    REQUIRES 
        edge_impulse 
        lvgl 
        esp_lvgl_port 
        esp32_s3_eye  
        nvs_flash
        esp_wifi
        esp_netif
        esp_event
        lwip
        esp_adc
)

# Reduce the level of paranoia to be able to compile sources
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-maybe-uninitialized
    -Wno-missing-field-initializers
    -Wno-error=sign-compare
    -Wno-error=double-promotion
    -Wno-type-limits)