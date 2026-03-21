#pragma once
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

void i2c_master_init(void);
esp_err_t qma_write(uint8_t reg, uint8_t data);
esp_err_t qma_read(uint8_t reg, uint8_t *data, size_t len);
void imu_read_g(float *xg, float *yg, float *zg);