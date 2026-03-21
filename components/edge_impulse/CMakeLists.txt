file(GLOB_RECURSE EI_SOURCES
    "edge-impulse-sdk/**/*.cpp"
    "edge-impulse-sdk/**/*.cc"
    "tflite-model/*.cpp"
)

idf_component_register(
    SRCS ${EI_SOURCES}
    INCLUDE_DIRS
        .
        edge-impulse-sdk
        edge-impulse-sdk/classifier
        edge-impulse-sdk/dsp
        edge-impulse-sdk/tensorflow
        edge-impulse-sdk/porting
        edge-impulse-sdk/porting/espressif
        model-parameters
        tflite-model
    REQUIRES
        esp_timer
        freertos
        driver
        esp-dsp
)

# prevent strict warnings inside EI/TFLM from stopping the build
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error
    -Wno-error=maybe-uninitialized
)